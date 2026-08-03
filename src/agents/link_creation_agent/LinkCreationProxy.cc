#include "LinkCreationProxy.h"

#include "LinkCreatorRegistry.h"
#include "ServiceBus.h"
#include "SystemParametersSingleton.h"

#include "Logger.h"

using namespace link_creation_agent;

// -------------------------------------------------------------------------------------------------
// Constructors, destructors and initialization

string LinkCreationProxy::PROCESS_QUERY_ANSWER = "process_query_answer";
string LinkCreationProxy::PROCESS_QUERY_ANSWER_RESPONSE = "process_query_answer_response";
    
string LinkCreationProxy::MAX_SUCCESSFUL_CREATES_PER_ROUND = "max_successful_creates_per_round";
string LinkCreationProxy::MAX_ROUNDS = "max_rounds";
string LinkCreationProxy::MAX_VISITS_PER_ROUND = "max_visits_per_round";
string LinkCreationProxy::MAX_UNPRODUCTIVE_ANSWERS_PER_ROUND = "max_unproductive_answers_per_round";

LinkCreationProxy::LinkCreationProxy() {
    // constructor typically used in processor
    lock_guard<mutex> semaphore(this->api_mutex);
    init();
    set_default_query_parameters();
}

LinkCreationProxy::LinkCreationProxy(
    const vector<string>& tokens,
    const string& context,
    const string& link_creator_tag,
    const shared_ptr<LinkCreator> link_creator = shared_ptr<LinkCreator>(nullptr))
    : BaseQueryProxy(tokens, context) {
    // constructor typically used in requestor
    init();
    set_default_query_parameters();
    this->link_creation_function_object = link_creation_function;
    set_link_creation_function_tag(link_creation_function_tag);
}

LinkCreationProxy::~LinkCreationProxy() {}

void LinkCreationProxy::init() {
    this->command = ServiceBus::LINK_CREATION;
    this->link_creation_function_object = shared_ptr<LinkCreator>(nullptr);
    this->ongoing_remote_link_creation = false;
    this->round_count = 0;
}

void LinkCreationProxy::set_default_query_parameters() {
    this->parameters = SystemParametersSingleton::get_instance()->get_link_creation_agent_params();
}

string LinkCreationProxy::to_string() {
    lock_guard<mutex> semaphore(this->api_mutex);
    string answer = "{BaseQueryProxy: ";
    answer += BaseQueryProxy::to_string();
    answer += ", link_creation_function: " + this->link_creation_function_tag;
    answer += "}";
    return answer;
}

// -------------------------------------------------------------------------------------------------
// Client-side API

void LinkCreationProxy::pack_command_line_args() { tokenize(this->args); }

void LinkCreationProxy::tokenize(vector<string>& output) {
    lock_guard<mutex> semaphore(this->api_mutex);
    output.insert(output.begin(), this->link_creation_function_tag);
    BaseQueryProxy::tokenize(output);
}

// -------------------------------------------------------------------------------------------------
// Server-side API

void LinkCreationProxy::untokenize(vector<string>& tokens) {
    BaseQueryProxy::untokenize(tokens);
    set_link_creation_function_tag(tokens[0]);
    tokens.erase(tokens.begin(), tokens.begin() + 1);
}

pair<unsigned int, unsigned int> LinkCreationProxy::link_creation(shared_ptr<QueryAnswer> answer) {
    if (this->link_creation_function_tag == "") {
        RAISE_ERROR("Invalid empty link creation function tag");
        return 0;
    } else if (this->link_creation_function_object == nullptr) {
        if (this->link_creation_function_tag == LinkCreatorFunctionRegistry::REMOTE_FUNCTION) {
            RAISE_ERROR("Invalid call to remote function");
        } else {
            RAISE_ERROR("Link creation function is not set up");
        }
        return 0;
    } else {
        return this->link_creation_function_object->create(answer);
    }
}

bool LinkCreationProxy::stop_criteria_met() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return (this->round_count >= this->parameters.get<unsigned int>(MAX_ROUNDS));
}

void LinkCreationProxy::set_link_creation_function_tag(const string& tag) {
    lock_guard<mutex> semaphore(this->api_mutex);
    if ((this->link_creation_function_tag != "") && (tag != this->link_creation_function_tag)) {
        RAISE_ERROR("Invalid reset of link creation function: " + this->link_creation_function_tag + " --> " + tag);
    } else {
        if (tag == "") {
            RAISE_ERROR("Invalid empty link creation function tag");
        }
        this->link_creation_function_tag = tag;
        if (tag != LinkCreatorFunctionRegistry::REMOTE_FUNCTION) {
            this->link_creation_function_object = LinkCreatorFunctionRegistry::function(tag);
        }
    }
}

bool LinkCreationProxy::is_link_creation_function_remote() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return (this->link_creation_function_object == nullptr) &&
           (this->link_creation_function_tag == LinkCreatorFunctionRegistry::REMOTE_FUNCTION);
}

void LinkCreationProxy::remote_link_creation(const vector<string>& answer_bundle) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->ongoing_remote_link_creation = true;
    this->remote_link_creation_result.clear();
    to_remote_peer(PROCESS_QUERY_ANSWER, answer_bundle);
}

bool LinkCreationProxy::remote_link_creation_finished() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return !this->ongoing_remote_link_creation;
}

vector<pair<unsigned int, unsigned int>> LinkCreationProxy::get_remotely_created_links() {
    lock_guard<mutex> semaphore(this->api_mutex);
    // This method doesn't return a reference to avoid concurrency hazard
    return this->remote_link_creation_result;
}

// ---------------------------------------------------------------------------------------------
// Virtual superclass API and the piggyback methods called by it

bool LinkCreationProxy::from_remote_peer(const string& command, const vector<string>& args) {
    LOG_DEBUG("Proxy command: <" << command << "> from " << this->peer_id() << " received in "
                                 << this->my_id());
    if (BaseQueryProxy::from_remote_peer(command, args)) {
        return true;
    } else if (command == PROCESS_QUERY_ANSWER) {
        process_query_answer(args);
        return true;
    } else if (command == PROCESS_QUERY_ANSWER_RESPONSE) {
        process_query_answer_response(args);
        return true;
    } else {
        RAISE_ERROR("Invalid LinkCreationProxy command: <" + command + ">");
        return false;
    }
}

void LinkCreationProxy::process_query_answer(const vector<string>& args) {
    lock_guard<mutex> semaphore(this->api_mutex);
    if (!this->is_aborting()) {
        if (args.size() == 0) {
            RAISE_ERROR("Invalid empty query answer bundle");
        } else {
            vector<string> bundle;
            for (auto tokens : args) {
                shared_ptr<QueryAnswer> query_answer = make_shared<QueryAnswer>();
                query_answer->untokenize(tokens);
                pair<unsigned int, unsigned int> stats = link_creation(query_answer);
                bundle.push_back(std::to_string(stats.first) + " " + std::to_string(stats.second));
            }
            to_remote_peer(PROCESS_QUERY_ANSWER_RESPONSE, bundle);
        }
    }
}

void LinkCreationProxy::process_query_answer_response(const vector<string>& args) {
    lock_guard<mutex> semaphore(this->api_mutex);
    if (!this->is_aborting()) {
        if (args.size() == 0) {
            RAISE_ERROR("Invalid empty link creation answer bundle");
        } else {
            for (auto value_str : args) {
                vector<string> value_vector = Utils::split(value_str);
                if ((value_vector.size != 2) || (value_vector[0] == "") || (value_vector[1] == "")) {
                    RAISE_ERROR("Invalid link creation answer: <" + value_str + ">");
                }
                this->remote_link_creation_result.push_back(make_pair(Utils::string_to_uint(value_vector[0]), Utils::string_to_uint(value_vector[1])));
            }
            this->ongoing_remote_link_creation = false;
        }
    }
}
