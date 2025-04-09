#include "ServiceBus.h"
#include "PatternMatchingQueryProxy.h"
#include "QueryNode.h"

using namespace query_engine;

string PatternMatchingQueryProxy::ABORT = "abort";

// -------------------------------------------------------------------------------------------------
// Public methods

PatternMatchingQueryProxy::PatternMatchingQueryProxy() {
    // constructor typically used in processor
    init();
}

PatternMatchingQueryProxy::PatternMatchingQueryProxy(
    const vector<string>& tokens,
    const string& context,
    bool update_attention_broker,
    bool count_only) 
    : BusCommandProxy() {

    // constructor typically used in requestor

    init();
    if (count_only) {
        this->command = ServiceBus::COUNTING_QUERY;
    } else {
        this->command = ServiceBus::PATTERN_MATCHING_QUERY;
    }
    this->count_flag = count_only;
    this->args = {context, to_string(update_attention_broker)};
    this->args.insert(this->args.end(), tokens.begin(), tokens.end());
}

void PatternMatchingQueryProxy::init() {
    this->answer_flow_finished = false;
    this->abort_flag = false;
    this->update_attention_broker = false;
    this->answer_count = 0;
    this->count_flag = false;
}

PatternMatchingQueryProxy::~PatternMatchingQueryProxy() {
}

bool PatternMatchingQueryProxy::is_aborting() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->abort_flag;
}

void PatternMatchingQueryProxy::abort() {
    // AQUI TODO Implementar
}

bool PatternMatchingQueryProxy::finished() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->answer_flow_finished &&
           (this->count_flag || (this->answer_queue.size() == 0));
}

unique_ptr<HandlesAnswer> PatternMatchingQueryProxy::pop() {
    lock_guard<mutex> semaphore(this->api_mutex);
    if (this->count_flag) {
        Utils::error("Can't pop QueryAnswers from count_only queries.");
    }
    return unique_ptr<HandlesAnswer>((HandlesAnswer*) this->answer_queue.dequeue());
}

unsigned int PatternMatchingQueryProxy::get_count() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->answer_count;
}

void PatternMatchingQueryProxy::query_answer_tokens_flow(const vector<string>& args) {
    lock_guard<mutex> semaphore(this->api_mutex);
    if (args.size() == 0) {
        Utils::error("Invalid empty query answer");
    }
    if (this->count_flag) {
        this->answer_count = (unsigned int) stoi(args[0]);
        this->answer_flow_finished = true;
    } else {
        for (auto tokens : args) {
            HandlesAnswer* query_answer = new HandlesAnswer();
            query_answer->untokenize(tokens);
            this->answer_queue.enqueue((void *) query_answer);
            this->answer_count++;
        }
    }
}

void PatternMatchingQueryProxy::query_answers_finished(const vector<string>& args) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->answer_flow_finished = true;
}

void PatternMatchingQueryProxy::abort(const vector<string>& args) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->abort_flag = true;
}

void PatternMatchingQueryProxy::from_remote_peer(const string& command, const vector<string>& args) {
    cout << "XXXXXX from_remote_peer() my_id: " << my_id() << " peer_id: " << peer_id() << endl;
    cout << "XXXXXX from_remote_peer() command: " << command << endl;
    if (command == QueryNode<HandlesAnswer>::QUERY_ANSWER_TOKENS_FLOW_COMMAND) {
        query_answer_tokens_flow(args);
    } else if (command == QueryNode<HandlesAnswer>::QUERY_ANSWERS_FINISHED_COMMAND) {
        query_answers_finished(args);
    } else if (command == ABORT) {
        abort(args);
    } else {
        Utils::error("Invalid proxy command: " + command);
    }
    cout << "XXXXXX from_remote_peer() Done" << endl;
}

shared_ptr<Message> BusCommandProxy::ProxyNode::message_factory(string& command, vector<string>& args) {
    std::shared_ptr<Message> message = BusCommandProxy::ProxyNode::message_factory(command, args);
    if (message) {
        return message;
    }
    if (command == QueryNode<HandlesAnswer>::QUERY_ANSWER_TOKENS_FLOW_COMMAND) {
        query_answer_tokens_flow(args);
    } else if (command == QueryNode<HandlesAnswer>::QUERY_ANSWERS_FINISHED_COMMAND) {
        query_answers_finished(args);
    }
    return std::shared_ptr<Message>(new DoNothing());
}

