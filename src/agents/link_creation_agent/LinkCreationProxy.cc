#include "LinkCreationProxy.h"

#include "LinkCreatorRegistry.h"
#include "Logger.h"
#include "ServiceBus.h"
#include "SystemParametersSingleton.h"

using namespace link_creation_agent;

// -------------------------------------------------------------------------------------------------
// Constructors, destructors and initialization

string LinkCreationProxy::MAX_SUCCESSFUL_CREATION_PER_ROUND = "max_successful_creation_per_round";
string LinkCreationProxy::MAX_UNPRODUCTIVE_VISITS_PER_ROUND = "max_unproductive_visits_per_round";
string LinkCreationProxy::MAX_VISIT_ATTEMPTS_PER_ROUND = "max_visit_attempts_per_round";
string LinkCreationProxy::MAX_ROUNDS = "max_rounds";

LinkCreationProxy::LinkCreationProxy() {
    // constructor typically used in processor
    lock_guard<mutex> semaphore(this->api_mutex);
    init();
    set_default_query_parameters();
}

LinkCreationProxy::LinkCreationProxy(const vector<string>& tokens,
                                     const string& context,
                                     const string& link_creator_tag,
                                     const shared_ptr<LinkCreator> link_creation_function)
    : BaseQueryProxy(tokens, context) {
    // constructor typically used in requestor
    init();
    set_default_query_parameters();
    this->link_creation_function_object = link_creation_function;
    set_link_creator_function_tag(link_creator_tag);
}

LinkCreationProxy::~LinkCreationProxy() {}

void LinkCreationProxy::init() {
    this->command = ServiceBus::LINK_CREATION;
    this->link_creation_function_object = shared_ptr<LinkCreator>(nullptr);
    this->round_count = 0;
}

void LinkCreationProxy::set_default_query_parameters() {
    this->parameters = SystemParametersSingleton::get_instance()->get_link_creation_agent_params();
}

string LinkCreationProxy::to_string() {
    lock_guard<mutex> semaphore(this->api_mutex);
    string answer = "{BaseQueryProxy: ";
    answer += BaseQueryProxy::to_string();
    answer += ", link_creator_function: " + this->link_creator_function_tag;
    answer += "}";
    return answer;
}

// -------------------------------------------------------------------------------------------------
// Client-side API

void LinkCreationProxy::pack_command_line_args() { tokenize(this->args); }

void LinkCreationProxy::tokenize(vector<string>& output) {
    lock_guard<mutex> semaphore(this->api_mutex);
    output.insert(output.begin(), this->link_creator_function_tag);
    BaseQueryProxy::tokenize(output);
}

// -------------------------------------------------------------------------------------------------
// Server-side API

void LinkCreationProxy::untokenize(vector<string>& tokens) {
    BaseQueryProxy::untokenize(tokens);
    if (tokens.size() < 1) {
        RAISE_ERROR("Invalid tokens for LinkCreationProxy");
    }
    set_link_creator_function_tag(tokens[0]);
    tokens.erase(tokens.begin(), tokens.begin() + 1);
}

LinkCreationStats LinkCreationProxy::link_creation(shared_ptr<QueryAnswer> answer) {
    LinkCreationStats stats;
    if (this->link_creator_function_tag == "") {
        RAISE_ERROR("Invalid empty link creation function tag");
    } else if (this->link_creation_function_object == nullptr) {
        if (this->link_creator_function_tag == LinkCreatorRegistry::REMOTE_FUNCTION) {
            RAISE_ERROR("Invalid call to remote function");
        } else {
            RAISE_ERROR("Link creation function is not set up");
        }
    } else {
        stats = this->link_creation_function_object->create(answer);
    }
    return stats;
}

bool LinkCreationProxy::stop_criteria_met() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return (this->round_count >= this->parameters.get<unsigned int>(MAX_ROUNDS));
}

void LinkCreationProxy::set_link_creator_function_tag(const string& tag) {
    lock_guard<mutex> semaphore(this->api_mutex);
    if (tag == LinkCreatorRegistry::REMOTE_FUNCTION) {
        RAISE_ERROR("Remote evaluation of link creators is not implemented yet.");
    }
    if ((this->link_creator_function_tag != "") && (tag != this->link_creator_function_tag)) {
        RAISE_ERROR("Invalid reset of link creation function: " + this->link_creator_function_tag +
                    " --> " + tag);
    } else {
        if (tag == "") {
            RAISE_ERROR("Invalid empty link creation function tag");
        }
        this->link_creator_function_tag = tag;
        if (tag != LinkCreatorRegistry::REMOTE_FUNCTION) {
            this->link_creation_function_object = LinkCreatorRegistry::function(tag);
        }
    }
}

void LinkCreationProxy::inc_round_count() {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->round_count++;
}

bool LinkCreationProxy::is_link_creation_function_remote() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return (this->link_creation_function_object == nullptr) &&
           (this->link_creator_function_tag == LinkCreatorRegistry::REMOTE_FUNCTION);
}

// ---------------------------------------------------------------------------------------------
// Virtual superclass API and the piggyback methods called by it

bool LinkCreationProxy::from_remote_peer(const string& command, const vector<string>& args) {
    LOG_DEBUG("Proxy command: <" << command << "> from " << this->peer_id() << " received in "
                                 << this->my_id());
    if (BaseQueryProxy::from_remote_peer(command, args)) {
        return true;
    } else {
        RAISE_ERROR("Invalid LinkCreationProxy command: <" + command + ">");
        return false;
    }
}
