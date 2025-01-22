#pragma once

#include <string>
#include <thread>
#include "DistributedAlgorithmNode.h"
#include "utils/queue.h"
#include "das_node/star_node.h"

using namespace std;
using namespace distributed_algorithm_node;

using namespace atom_space_node;

namespace query_node
{

    /**
     * @brief Query Node class
     */
    class QueryNode : public StarNode
    {

    public:
        QueryNode(
            const string &node_id,
            bool is_server);
        virtual ~QueryNode();
        virtual void graceful_shutdown();
        bool is_shutting_down();
        void query_answers_finished();
        bool is_query_answers_finished();
        void add_answer(string handler);
        string *pop_query_answer();
        bool is_query_answers_empty();
        void query_answer_processor_method();

        const string QUERY_ANSWER_FLOW_COMMAND = "query_answer_tokens_flow";
        const string QUERY_ANSWER_TOKENS_FLOW_COMMAND = "query_answer_flow";
        const string QUERY_ANSWERS_FINISHED_COMMAND = "query_answers_finished";

    protected:
        Queue<void *> query_answer_queue;
        thread *query_answer_processor;
        bool requires_serialization;

    private:
        bool is_server;
        bool shutdown_flag;
        mutex shutdown_flag_mutex;
        bool query_answers_finished_flag;
        mutex query_answers_finished_flag_mutex;
    };

    class QueryAnswerFlow
    {

    public:
        QueryAnswerFlow(string command, vector<string> &args);
        void act(shared_ptr<MessageFactory> node);

    private:
        vector<string *> query_answers;
    };

    class QueryAnswerTokensFlow
    {

    public:
        QueryAnswerTokensFlow(string command, vector<string> &args);
        void act(shared_ptr<MessageFactory> node);

    private:
        vector<string> query_answers_tokens;
    };

    class QueryAnswersFinished
    {

    public:
        QueryAnswersFinished(string command, vector<string> &args);
        void act(shared_ptr<MessageFactory> node);
    };

} // namespace query_node
