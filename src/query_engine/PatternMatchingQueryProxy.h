#pragma once

#include <mutex>
#include "BusCommandProxy.h"
#include "SharedQueue.h"
#include "HandlesAnswer.h"
#include "Message.h"

using namespace std;
using namespace service_bus;
using namespace distributed_algorithm_node;

namespace query_engine {

/**
 *
 */
class PatternMatchingQueryProxy : public BusCommandProxy {

public:

    PatternMatchingQueryProxy();
    PatternMatchingQueryProxy(
        const vector<string>& tokens,
        const string& context = "",
        bool update_attention_broker = false,
        bool count_only = false);
    ~PatternMatchingQueryProxy();

    bool is_aborting();
    void abort();

    void from_remote_peer(const string& command, const vector<string>& args);
    void query_answer_tokens_flow(const vector<string>& args);
    void query_answers_finished(const vector<string>& args);
    void abort(const vector<string>& args);

    virtual shared_ptr<Message> message_factory(string& command, vector<string>& args);

    bool finished();
    unique_ptr<HandlesAnswer> pop();
    unsigned int get_count();

    static string ABORT;
    string context;
    bool update_attention_broker;
    vector<string> query_tokens;
    bool count_flag;

private:

    bool abort_flag;
    mutex api_mutex;
    SharedQueue answer_queue;
    unsigned int answer_count;
    bool answer_flow_finished;

    void init();
};

} // namespace query_engine
