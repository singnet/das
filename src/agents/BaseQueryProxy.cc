#include "BaseQueryProxy.h"

#include "AtomDBUtils.h"
#include "Logger.h"
#include "ServiceBus.h"
#include "SystemParametersSingleton.h"

using namespace agents;

// -------------------------------------------------------------------------------------------------
// Constructors, destructors and initialization

string BaseQueryProxy::ABORT = "abort";
string BaseQueryProxy::ANSWER_BUNDLE = "answer_bundle";
string BaseQueryProxy::SUPPLEMENTARY_INFO = "supplementary_info";
string BaseQueryProxy::FINISHED = "finished";

string BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG = "unique_assignment_flag";
string BaseQueryProxy::ATTENTION_UPDATE = "attention_update";
string BaseQueryProxy::ATTENTION_CORRELATION = "attention_correlation";
string BaseQueryProxy::ATTENTION_FOCUS_STRICTNESS = "attention_focus_strictness";
string BaseQueryProxy::MAX_BUNDLE_SIZE = "max_bundle_size";
string BaseQueryProxy::MAX_ANSWERS = "max_answers";
string BaseQueryProxy::POPULATE_METTA_MAPPING = "populate_metta_mapping";
string BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS = "use_metta_as_query_tokens";
string BaseQueryProxy::ALLOW_INCOMPLETE_CHAIN_PATH = "allow_incomplete_chain_path";
string BaseQueryProxy::PUBLIC_KEY_TOKENS = "public_key_tokens";

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
    this->answer_count = 0;
    this->parameters += SystemParametersSingleton::get_instance()->get_base_query_params();
    this->_keychain = nullptr;
    this->_atomdb = AtomDBSingleton::get_instance();
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

vector<string> BaseQueryProxy::get_supplementary_info(SUPPLEMENTARY_INFO_TAG tag) {
    lock_guard<recursive_mutex> semaphore(this->api_mutex);
    string key = supplementary_info_tag_to_string(tag);
    if (this->_supplementary_info.find(key) == this->_supplementary_info.end()) {
        return {};
    } else {
        return this->_supplementary_info[key];
    }
}

void BaseQueryProxy::clear_supplementary_info(SUPPLEMENTARY_INFO_TAG tag) {
    lock_guard<recursive_mutex> semaphore(this->api_mutex);
    if (tag == UNDEFINED) {
        this->_supplementary_info.clear();
    } else {
        string key = supplementary_info_tag_to_string(tag);
        if (this->_supplementary_info.find(key) != this->_supplementary_info.end()) {
            this->_supplementary_info[key].clear();
        }
    }
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

void BaseQueryProxy::push_supplementary_info(SUPPLEMENTARY_INFO_TAG tag, const string& handle) {
    lock_guard<recursive_mutex> semaphore(this->api_mutex);
    this->supplementary_info_bundle_vector.push_back(supplementary_info_tag_to_string(tag));
    this->supplementary_info_bundle_vector.push_back(handle);
    LOG_DEBUG("Supplementary info pushed to bundle: " + supplementary_info_tag_to_string(tag) + " -> " +
              handle);
}

void BaseQueryProxy::flush_answer_bundle() {
    lock_guard<recursive_mutex> semaphore(this->api_mutex);
    if (this->supplementary_info_bundle_vector.size() > 0) {
        LOG_DEBUG("Flushing " << this->supplementary_info_bundle_vector.size()
                              << " tokens in supplementary info bundle");
        to_remote_peer(SUPPLEMENTARY_INFO, this->supplementary_info_bundle_vector);
        this->supplementary_info_bundle_vector.clear();
    }
    if (this->answer_bundle_vector.size() > 0) {
        LOG_DEBUG("Flushing " << this->answer_bundle_vector.size() << " answers in bundle");
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

    string keychain_tokens = this->parameters.get_or<string>(PUBLIC_KEY_TOKENS, "");
    if (keychain_tokens != "") {
        auto __keychain = make_shared<Keychain>();
        __keychain->untokenize(Utils::split(keychain_tokens));
        this->_keychain = __keychain;
    }
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

void BaseQueryProxy::populate_metta_mapping(QueryAnswer* answer) {
    for (string& handle : answer->get_handles_vector()) {
        AtomDBUtils::handle_to_metta(handle, answer->metta_expression, _keychain);
    }
    for (unsigned int i = 0; i < answer->get_paths_size(); i++) {
        for (string& handle : answer->get_path_vector(i)) {
            AtomDBUtils::handle_to_metta(handle, answer->metta_expression, _keychain);
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
        } else if (command == SUPPLEMENTARY_INFO) {
            supplementary_info(args);
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

void BaseQueryProxy::supplementary_info(const vector<string>& args) {
    lock_guard<recursive_mutex> semaphore(this->api_mutex);
    if (!this->is_aborting()) {
        if (args.size() == 0) {
            LOG_INFO("Disregarding empty supplementary info bundle");
        } else {
            if ((args.size() % 2) != 0) {
                RAISE_ERROR(
                    "Invalid tokens for supplementary info bundle. Expected an even number of tokens. "
                    "Tokens: [" +
                    Utils::join(args, ", ") + "]");
            }
            for (unsigned int i = 0; i < args.size(); i += 2) {
                string key = args[i];
                if (!supplementary_info_tag_is_valid(key)) {
                    RAISE_ERROR("Invalid supplementary info key: " + key);
                }
                this->_supplementary_info[key].push_back(args[i + 1]);
            }
        }
    }
}

void BaseQueryProxy::build_supplementary_info() {
    supplementary_info(this->supplementary_info_bundle_vector);
}
