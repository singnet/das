#include "BaseQueryProxy.h"

#include "Logger.h"
#include "ServiceBus.h"
#include "SystemParametersSingleton.h"

using namespace agents;

// -------------------------------------------------------------------------------------------------
// Constructors, destructors and initialization

string BaseQueryProxy::ABORT = "abort";
string BaseQueryProxy::ANSWER_BUNDLE = "answer_bundle";
string BaseQueryProxy::BUILT_ATOMS_BUNDLE = "built_atoms_bundle";
string BaseQueryProxy::FINISHED = "finished";

string BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG = "unique_assignment_flag";
string BaseQueryProxy::ATTENTION_UPDATE = "attention_update";
string BaseQueryProxy::ATTENTION_CORRELATION = "attention_correlation";
string BaseQueryProxy::ATTENTION_FOCUS_STRICTNESS = "attention_focus_strictness";
string BaseQueryProxy::MAX_BUNDLE_SIZE = "max_bundle_size";
string BaseQueryProxy::MAX_ANSWERS = "max_answers";
string BaseQueryProxy::USE_LINK_TEMPLATE_CACHE = "use_link_template_cache";
string BaseQueryProxy::POPULATE_METTA_MAPPING = "populate_metta_mapping";
string BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS = "use_metta_as_query_tokens";
string BaseQueryProxy::ALLOW_INCOMPLETE_CHAIN_PATH = "allow_incomplete_chain_path";

BaseQueryProxy::BaseQueryProxy() {
    // constructor typically used in processor
    lock_guard<recursive_mutex> semaphore(this->api_mutex);
    init();
}

BaseQueryProxy::BaseQueryProxy(const vector<string>& tokens, const string& context) : BaseProxy() {
    // constructor typically used in requestor
    lock_guard<recursive_mutex> semaphore(this->api_mutex);
    init();
    this->context = context;
    this->query_tokens.insert(this->query_tokens.end(), tokens.begin(), tokens.end());
}

void BaseQueryProxy::init() {
    this->atomdb = AtomDBSingleton::get_instance();
    this->answer_count = 0;
    this->parameters += SystemParametersSingleton::get_instance()->get_base_query_params();
}

BaseQueryProxy::~BaseQueryProxy() {}

// -------------------------------------------------------------------------------------------------
// Client-side API

shared_ptr<QueryAnswer> BaseQueryProxy::pop() {
    lock_guard<recursive_mutex> semaphore(this->api_mutex);
    if (this->is_aborting()) {
        return shared_ptr<QueryAnswer>(NULL);
    } else {
        return shared_ptr<QueryAnswer>((QueryAnswer*) this->answer_queue.dequeue());
    }
}

unsigned int BaseQueryProxy::get_count() {
    lock_guard<recursive_mutex> semaphore(this->api_mutex);
    return this->answer_count;
}

void BaseQueryProxy::set_count(unsigned int count) {
    lock_guard<recursive_mutex> semaphore(this->api_mutex);
    this->answer_count = count;
}

void BaseQueryProxy::tokenize(vector<string>& output) {
    lock_guard<recursive_mutex> semaphore(this->api_mutex);
    output.insert(output.begin(), this->query_tokens.begin(), this->query_tokens.end());
    output.insert(output.begin(), std::to_string(this->query_tokens.size()));
    output.insert(output.begin(), this->get_context());
    BaseProxy::tokenize(output);
}

bool BaseQueryProxy::finished() {
    lock_guard<recursive_mutex> semaphore(this->api_mutex);
    return (this->is_aborting() || (BaseProxy::finished() && this->answer_queue.empty()));
}

bool BaseQueryProxy::finished_cycle() {
    lock_guard<recursive_mutex> semaphore(this->api_mutex);
    return (this->is_aborting() || (BaseProxy::finished_cycle() && this->answer_queue.empty()));
}

vector<string> BaseQueryProxy::get_built_atoms() {
    lock_guard<recursive_mutex> semaphore(this->api_mutex);
    return this->built_atoms;
}

// -------------------------------------------------------------------------------------------------
// Server-side API

void BaseQueryProxy::push(shared_ptr<QueryAnswer> answer) {
    lock_guard<recursive_mutex> semaphore(this->api_mutex);
    this->answer_bundle_vector.push_back(answer->tokenize());
    LOG_DEBUG("Answer pushed to bundle: " + answer->to_string() + " tokens: [" +
              this->answer_bundle_vector.back() + "]");
    if (this->answer_bundle_vector.size() >= this->parameters.get<unsigned int>(MAX_BUNDLE_SIZE)) {
        flush_answer_bundle();
    }
}

void BaseQueryProxy::push_built_atom(const string& handle) {
    lock_guard<recursive_mutex> semaphore(this->api_mutex);
    this->built_atoms_bundle_vector.push_back(handle);
    LOG_DEBUG("Built atom pushed to bundle: " + handle);
}

void BaseQueryProxy::flush_answer_bundle() {
    lock_guard<recursive_mutex> semaphore(this->api_mutex);
    if (this->built_atoms_bundle_vector.size() > 0) {
        LOG_DEBUG("Flushing " << this->built_atoms_bundle_vector.size() << " answers in bundle");
        to_remote_peer(BUILT_ATOMS_BUNDLE, this->built_atoms_bundle_vector);
        this->built_atoms_bundle_vector.clear();
    }
    if (this->answer_bundle_vector.size() > 0) {
        LOG_DEBUG("Flushing " << this->answer_bundle_vector.size() << " atoms in bundle");
        to_remote_peer(ANSWER_BUNDLE, this->answer_bundle_vector);
        this->answer_bundle_vector.clear();
    }
    Utils::sleep();  // TODO remove this
}

void BaseQueryProxy::query_processing_finished() {
    lock_guard<recursive_mutex> semaphore(this->api_mutex);
    flush_answer_bundle();
    to_remote_peer(FINISHED, {});
}

void BaseQueryProxy::untokenize(vector<string>& tokens) {
    lock_guard<recursive_mutex> semaphore(this->api_mutex);
    BaseProxy::untokenize(tokens);
    this->context = tokens[0];
    unsigned int num_query_tokens = std::stoi(tokens[1]);

    this->query_tokens.insert(
        this->query_tokens.begin(), tokens.begin() + 2, tokens.begin() + 2 + num_query_tokens);
    tokens.erase(tokens.begin(), tokens.begin() + 2 + num_query_tokens);
}

const string& BaseQueryProxy::get_context() {
    lock_guard<recursive_mutex> semaphore(this->api_mutex);
    return this->context;
}

const vector<string>& BaseQueryProxy::get_query_tokens() {
    lock_guard<recursive_mutex> semaphore(this->api_mutex);
    return this->query_tokens;
}

string BaseQueryProxy::to_string() {
    lock_guard<recursive_mutex> semaphore(this->api_mutex);
    string answer = "{";
    answer += "context: " + this->get_context();
    answer += ", tokens: [";
    bool empty_flag = true;
    for (auto token : this->query_tokens) {
        answer += token + ", ";
        empty_flag = false;
    }
    if (!empty_flag) {
        answer.pop_back();
        answer.pop_back();
    }
    answer += "], BaseProxy: ";
    answer += BaseProxy::to_string();
    answer += "}";
    return answer;
}

void BaseQueryProxy::recursive_metta_mapping(string handle, map<string, string>& table) {
    if (table.find(handle) == table.end()) {
        auto atom = this->atomdb->get_atom(handle);
        if (atom->arity() > 0) {
            // is link
            auto link = dynamic_cast<Link*>(atom.get());
            if (link->type != "Expression") {
                RAISE_ERROR("Link type \"" + link->type + "\" can't be mapped to MeTTa");
                table[handle] = "";
                return;
            }
            unsigned int arity = link->arity();
            for (unsigned int i = 0; i < arity; i++) {
                recursive_metta_mapping(link->targets[i], table);
            }
            string expression = "(";
            bool empty_flag = true;
            for (unsigned int i = 0; i < arity; i++) {
                expression += table[link->targets[i]];
                expression += " ";
                empty_flag = false;
            }
            if (!empty_flag) {
                expression.pop_back();
            }
            expression += ")";
            table[handle] = expression;
        } else {
            // is node
            auto node = dynamic_cast<Node*>(atom.get());
            if (node->type != "Symbol") {
                RAISE_ERROR("Node type \"" + node->type + "\" can't be mapped to MeTTa");
                table[handle] = "";
                return;
            }
            table[handle] = node->name;
        }
    }
}

void BaseQueryProxy::populate_metta_mapping(QueryAnswer* answer) {
    for (string& handle : answer->get_handles_vector()) {
        recursive_metta_mapping(handle, answer->metta_expression);
    }
    for (unsigned int i = 0; i < answer->get_paths_size(); i++) {
        for (string& handle : answer->get_path_vector(i)) {
            recursive_metta_mapping(handle, answer->metta_expression);
        }
    }
}

// ---------------------------------------------------------------------------------------------
// Virtual superclass API and the piggyback methods called by it

bool BaseQueryProxy::from_remote_peer(const string& command, const vector<string>& args) {
    LOG_DEBUG("Proxy command: <" << command << "> from " << this->peer_id() << " received in "
                                 << this->my_id());
    if (BaseProxy::from_remote_peer(command, args)) {
        return true;
    } else {
        if (command == ANSWER_BUNDLE) {
            answer_bundle(args);
        } else if (command == BUILT_ATOMS_BUNDLE) {
            built_atoms_bundle(args);
        } else {
            return false;
        }
        return true;
    }
}

void BaseQueryProxy::cycle_ended() {
    flush_answer_bundle();
    BaseProxy::cycle_ended();
}

void BaseQueryProxy::answer_bundle(const vector<string>& args) {
    lock_guard<recursive_mutex> semaphore(this->api_mutex);
    if (!this->is_aborting()) {
        if (args.size() == 0) {
            LOG_INFO("Disregarding empty query answer bundle");
        } else {
            for (auto tokens : args) {
                QueryAnswer* query_answer = new QueryAnswer();
                query_answer->untokenize(tokens);
                this->answer_queue.enqueue((void*) query_answer);
                this->answer_count++;
            }
        }
    }
}

void BaseQueryProxy::built_atoms_bundle(const vector<string>& args) {
    lock_guard<recursive_mutex> semaphore(this->api_mutex);
    if (!this->is_aborting()) {
        if (args.size() == 0) {
            LOG_INFO("Disregarding empty built atoms answer bundle");
        } else {
            for (auto handle : args) {
                this->built_atoms.push_back(handle);
            }
        }
    }
}
