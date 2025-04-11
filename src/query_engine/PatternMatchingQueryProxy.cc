#include "ServiceBus.h"
#include "PatternMatchingQueryProxy.h"

using namespace query_engine;

string PatternMatchingQueryProxy::ABORT = "abort";
string PatternMatchingQueryProxy::ANSWER_BUNDLE = "answer_bundle";
string PatternMatchingQueryProxy::COUNT = "count";
string PatternMatchingQueryProxy::FINISHED = "finished";

// -------------------------------------------------------------------------------------------------
// Public methods

PatternMatchingQueryProxy::PatternMatchingQueryProxy() {
    // constructor typically used in processor
    lock_guard<mutex> semaphore(this->api_mutex);
    init();
}

PatternMatchingQueryProxy::PatternMatchingQueryProxy(
    const vector<string>& tokens,
    const string& context,
    bool update_attention_broker,
    bool count_only) 
    : BusCommandProxy() {

    // constructor typically used in requestor

    lock_guard<mutex> semaphore(this->api_mutex);
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
    lock_guard<mutex> semaphore(this->api_mutex);
    // AQUI TODO Implementar
}

const string& PatternMatchingQueryProxy::get_context() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->context;
}

void PatternMatchingQueryProxy::set_context(const string& context) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->context = context;
}

bool PatternMatchingQueryProxy::get_attention_update_flag() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->update_attention_broker;
}

void PatternMatchingQueryProxy::set_attention_update_flag(bool flag) {
    lock_guard<mutex> semaphore(this->api_mutex);
    this->update_attention_broker = flag;
}

const vector<string>& PatternMatchingQueryProxy::get_query_tokens() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->query_tokens;
}

bool PatternMatchingQueryProxy::get_count_flag() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->count_flag;
}

bool PatternMatchingQueryProxy::finished() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->answer_flow_finished &&
           (this->count_flag || (this->answer_queue.size() == 0));
}

shared_ptr<QueryAnswer> PatternMatchingQueryProxy::pop() {
    lock_guard<mutex> semaphore(this->api_mutex);
    if (this->count_flag) {
        Utils::error("Can't pop QueryAnswers from count_only queries.");
    }
    return shared_ptr<QueryAnswer>((QueryAnswer*) this->answer_queue.dequeue());
}

unsigned int PatternMatchingQueryProxy::get_count() {
    lock_guard<mutex> semaphore(this->api_mutex);
    return this->answer_count;
}

void PatternMatchingQueryProxy::answer_bundle(const vector<string>& args) {
    lock_guard<mutex> semaphore(this->api_mutex);
    if (args.size() == 0) {
        Utils::error("Invalid empty query answer");
    }
    if (this->count_flag) {
        Utils::error("Invalid answer_bundle command. Query is count_only.");
    } else {
        for (auto tokens : args) {
            QueryAnswer* query_answer = new QueryAnswer();
            query_answer->untokenize(tokens);
            this->answer_queue.enqueue((void *) query_answer);
            this->answer_count++;
        }
    }
}

void PatternMatchingQueryProxy::count_answer(const vector<string>& args) {
    lock_guard<mutex> semaphore(this->api_mutex);
    if (args.size() != 1) {
        Utils::error("Invalid args for count command");
    }
    if (! this->count_flag) {
        Utils::error("Invalid count command. Query is not count_only.");
    } else {
        this->answer_count = stoi(args[0]);
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
    lock_guard<mutex> semaphore(this->api_mutex);
    cout << "XXXXXX from_remote_peer() my_id: " << my_id() << " peer_id: " << peer_id() << endl;
    cout << "XXXXXX from_remote_peer() command: " << command << endl;
    if (command == ANSWER_BUNDLE) {
        answer_bundle(args);
    } else if (command == COUNT) {
        count_answer(args);
    } else if (command == FINISHED) {
        query_answers_finished(args);
    } else if (command == ABORT) {
        abort(args);
    } else {
        Utils::error("Invalid proxy command: " + command);
    }
    cout << "XXXXXX from_remote_peer() Done" << endl;
}
