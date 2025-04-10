#pragma once

#include <mutex>
#include "BusCommandProxy.h"
#include "QueryAnswer.h"
#include "SharedQueue.h"
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
    const string& get_context();
    bool get_attention_update_flag();
    const vector<string>& get_query_tokens();
    bool get_count_flag();

    void answer_bundle(const vector<string>& args);
    void count_answer(const vector<string>& args);
    void query_answers_finished(const vector<string>& args);
    void abort(const vector<string>& args);
    void from_remote_peer(const string& command, const vector<string>& args) override;

    bool finished();
    unique_ptr<QueryAnswer> pop();
    unsigned int get_count();

    static string ABORT;
    static string ANSWER_BUNDLE;
    static string COUNT;
    static string FINISHED;

private:

    mutex api_mutex;
    bool abort_flag;
    SharedQueue answer_queue;
    unsigned int answer_count;
    bool answer_flow_finished;
    string context;
    bool update_attention_broker;
    vector<string> query_tokens;
    bool count_flag;

    void init();
};

} // namespace query_engine
