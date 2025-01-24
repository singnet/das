#pragma once

#include "star_node.h"
#include "queue.h"

using namespace std;
using namespace atom_space_node;

namespace query_node {

    class QueryNodeServer : public StarNode {
        public:
            QueryNodeServer(const string& node_id);
            ~QueryNodeServer();
            void add_request(string& request);
            string pop_request();
            bool is_query_empty();
            bool is_shutting_down();
            virtual shared_ptr<Message> message_factory(string& command, vector<string>& args);
        private:
            Queue<string> request_queue;
            const string QUERY_ANSWER_TOKENS_FLOW_COMMAND = "query_answer_tokens_flow";
            const string QUERY_ANSWER_FLOW_COMMAND = "query_answer_flow";
            const string QUERY_ANSWERS_FINISHED_COMMAND = "query_answers_finished";
            bool shutting_down = false;
    };

    class QueryAnswerTokensFlow : public Message {
        public:
            string command;
            vector<string> args;
            QueryAnswerTokensFlow(string command, vector<string>& args);
            void act(shared_ptr<MessageFactory> node);
    };

    class QueryAnswerFlow : public Message {
        public:
            string command;
            vector<string> args;
            QueryAnswerFlow(string command, vector<string>& args);
            void act(shared_ptr<MessageFactory> node);
    };

    class QueryAnswersFinished : public Message {
        public:
            string command;
            vector<string> args;
            QueryAnswersFinished(string command, vector<string>& args);
            void act(shared_ptr<MessageFactory> node);
    };
}