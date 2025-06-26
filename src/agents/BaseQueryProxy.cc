#include "BaseQueryProxy.h"

#include "ServiceBus.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace agents;

// -------------------------------------------------------------------------------------------------
// Constructors, destructors and initialization

string BaseQueryProxy::ABORT = "abort";
string BaseQueryProxy::ANSWER_BUNDLE = "answer_bundle";
string BaseQueryProxy::FINISHED = "finished";

string BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG = "unique_assignment_flag";
string BaseQueryProxy::ATTENTION_UPDATE_FLAG = "attention_update_flag";
string BaseQueryProxy::MAX_BUNDLE_SIZE = "max_bundle_size";

BaseQueryProxy::BaseQueryProxy() {
    // constructor typically used in processor
    lock_guard<mutex> semaphore(this->api_mutex);
    init();
}

BaseQueryProxy::BaseQueryProxy(const vector<string>& tokens, const string& context) : BaseProxy() {
    // constructor typically used in requestor
    lock_guard<mutex> semaphore(this->api_mutex);
    init();
    this->context = context;
    this->query_tokens.insert(this->query_tokens.end(), tokens.begin(), tokens.end());
}

void BaseQueryProxy::init() {
    this->answer_count = 0;
    this->parameters[UNIQUE_ASSIGNMENT_FLAG] = false;
    this->parameters[ATTENTION_UPDATE_FLAG] = false;
    this->parameters[MAX_BUNDLE_SIZE] = (unsigned int) 1000;
}

BaseQueryProxy::~BaseQueryProxy() {}

// -------------------------------------------------------------------------------------------------
// Client-side API

shared_ptr<QueryAnswer> BaseQueryProxy::pop() {
    lock_guard<mutex> semaphore(this->api_mutex);
    if (this->is_aborting()) {
        return shared_ptr<QueryAnswer>(NULL);
    } else {
        return shared_ptr<QueryAnswer>((QueryAnswer*) this->answer_queue.dequeue());
    }
}

unsigned int BaseQueryProxy::get_count() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->answer_count;
}

void BaseQueryProxy::set_count(unsigned int count) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->answer_count = count;
}

void BaseQueryProxy::tokenize(vector<string>& output) {
    output.insert(output.begin(), this->query_tokens.begin(), this->query_tokens.end());
    output.insert(output.begin(), std::to_string(this->get_query_tokens().size()));
    output.insert(output.begin(), this->get_context());
    BaseProxy::tokenize(output);
}

// -------------------------------------------------------------------------------------------------
// Server-side API

void BaseQueryProxy::push(shared_ptr<QueryAnswer> answer) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->answer_bundle_vector.push_back(answer->tokenize());
    if (this->answer_bundle_vector.size() >= this->parameters.get<unsigned int>(MAX_BUNDLE_SIZE)) {
        flush_answer_bundle();
    }
}

void BaseQueryProxy::flush_answer_bundle() {
    if (this->answer_bundle_vector.size() > 0) {
        to_remote_peer(ANSWER_BUNDLE, this->answer_bundle_vector);
        this->answer_bundle_vector.clear();
    }
    Utils::sleep();  // TODO remove this
}

void BaseQueryProxy::query_processing_finished() {
    flush_answer_bundle();
    to_remote_peer(FINISHED, {});
}

void BaseQueryProxy::untokenize(vector<string>& tokens) {
    BaseProxy::untokenize(tokens);
    this->context = tokens[0];
    unsigned int num_query_tokens = std::stoi(tokens[1]);

    this->query_tokens.insert(
        this->query_tokens.begin(), tokens.begin() + 2, tokens.begin() + 2 + num_query_tokens);
    tokens.erase(tokens.begin(), tokens.begin() + 2 + num_query_tokens);
}

const string& BaseQueryProxy::get_context() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->context;
}

const vector<string>& BaseQueryProxy::get_query_tokens() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->query_tokens;
}

string BaseQueryProxy::to_string() {
    string answer = "{";
    answer += "context: " + this->get_context();
    answer += " " + BaseProxy::to_string();
    answer += "}";
    return answer;
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
        } else {
            return false;
        }
        return true;
    }
}

void BaseQueryProxy::answer_bundle(const vector<string>& args) {
    lock_guard<mutex> semaphore(this->api_mutex);
    if (!this->is_aborting()) {
        if (args.size() == 0) {
            Utils::error("Invalid empty query answer");
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
