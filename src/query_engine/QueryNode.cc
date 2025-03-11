#pragma once

#include "QueryNode.h"

#include "CountAnswer.h"
#include "HandlesAnswer.h"
#include "LeadershipBroker.h"
#include "MessageBroker.h"
#include "Utils.h"

using namespace query_node;
using namespace std;

// --------------------------------------------------------------------------------
// Public methods

template <class AnswerType>
QueryNode<AnswerType>::QueryNode(const string& node_id,
                                 bool is_server,
                                 MessageBrokerType messaging_backend)
    : DistributedAlgorithmNode(node_id, LeadershipBrokerType::SINGLE_MASTER_SERVER, messaging_backend) {
    this->is_server = is_server;
    this->query_answer_processor = NULL;
    this->query_answers_finished_flag = false;
    this->shutdown_flag = false;
    this->work_done_flag = false;
    if (messaging_backend == MessageBrokerType::RAM) {
        this->requires_serialization = false;
    } else {
        this->requires_serialization = true;
    }
}

template <class AnswerType>
QueryNode<AnswerType>::~QueryNode() {
    this->graceful_shutdown();
    while (!this->query_answer_queue.empty()) {
        delete (QueryAnswer*) this->query_answer_queue.dequeue();
    }
}

template <class AnswerType>
void QueryNode<AnswerType>::graceful_shutdown() {
    if (is_shutting_down()) {
        return;
    }
    DistributedAlgorithmNode::graceful_shutdown();
    this->shutdown_flag_mutex.lock();
    this->shutdown_flag = true;
    this->shutdown_flag_mutex.unlock();
    if (this->query_answer_processor != NULL) {
        this->query_answer_processor->join();
        delete this->query_answer_processor;
        this->query_answer_processor = NULL;
    }
}

template <class AnswerType>
bool QueryNode<AnswerType>::is_shutting_down() {
    bool answer;
    this->shutdown_flag_mutex.lock();
    answer = this->shutdown_flag;
    this->shutdown_flag_mutex.unlock();
    return answer;
}

template <class AnswerType>
void QueryNode<AnswerType>::query_answers_finished() {
    this->query_answers_finished_flag_mutex.lock();
    this->query_answers_finished_flag = true;
    this->query_answers_finished_flag_mutex.unlock();
}

template <class AnswerType>
bool QueryNode<AnswerType>::is_query_answers_finished() {
    bool answer;
    this->query_answers_finished_flag_mutex.lock();
    answer = this->query_answers_finished_flag;
    this->query_answers_finished_flag_mutex.unlock();
    return answer;
}

template <class AnswerType>
shared_ptr<Message> QueryNode<AnswerType>::message_factory(string& command, vector<string>& args) {
    std::shared_ptr<Message> message = DistributedAlgorithmNode::message_factory(command, args);
    if (message) {
        return message;
    }
    if (command == QUERY_ANSWER_FLOW_COMMAND) {
        return make_shared<QueryAnswerFlow<AnswerType>>(command, args);
    } else if (command == QUERY_ANSWER_TOKENS_FLOW_COMMAND) {
        return make_shared<QueryAnswerTokensFlow<AnswerType>>(command, args);
    } else if (command == QUERY_ANSWERS_FINISHED_COMMAND) {
        return make_shared<QueryAnswersFinished<AnswerType>>(command, args);
    }
    return std::shared_ptr<Message>{};
}

template <class AnswerType>
void QueryNode<AnswerType>::add_query_answer(QueryAnswer* query_answer) {
    if (is_query_answers_finished()) {
        Utils::error("Invalid addition of new query answer.");
    } else {
        this->query_answer_queue.enqueue((void*) query_answer);
    }
}

template <class AnswerType>
QueryAnswer* QueryNode<AnswerType>::pop_query_answer() {
    return (QueryAnswer*) this->query_answer_queue.dequeue();
}

template <class AnswerType>
bool QueryNode<AnswerType>::is_query_answers_empty() {
    return this->query_answer_queue.empty();
}

template <class AnswerType>
QueryNodeServer<AnswerType>::QueryNodeServer(const string& node_id, MessageBrokerType messaging_backend)
    : QueryNode<AnswerType>(node_id, true, messaging_backend) {
    this->join_network();
    this->query_answer_processor =
        new thread(&QueryNodeServer<AnswerType>::query_answer_processor_method, this);
}

template <class AnswerType>
void QueryNodeServer<AnswerType>::node_joined_network(const string& node_id) {
    this->add_peer(node_id);
}

template <class AnswerType>
string QueryNodeServer<AnswerType>::cast_leadership_vote() {
    return this->node_id();
}

template <class AnswerType>
void QueryNodeServer<AnswerType>::query_answer_processor_method() {
    while (!this->is_shutting_down()) {
        Utils::sleep();
    }
}

template <class AnswerType>
void QueryNodeClient<AnswerType>::query_answer_processor_method() {
    QueryAnswer* query_answer;
    vector<string> args;
    bool answers_finished_flag = false;
    while (!this->is_shutting_down()) {
        while ((query_answer = (QueryAnswer*) this->query_answer_queue.dequeue()) != NULL) {
            if (this->requires_serialization) {
                string tokens = query_answer->tokenize();
                args.push_back(tokens);
            } else {
                args.push_back(to_string((unsigned long) query_answer));
            }
        }
        if (args.empty()) {
            // The order of the AND clauses below matters
            if (!answers_finished_flag && this->is_query_answers_finished() &&
                this->query_answer_queue.empty()) {
                this->send(QUERY_ANSWERS_FINISHED_COMMAND, args, this->server_id);
                answers_finished_flag = true;
                this->work_done_flag = true;
            }
        } else {
            if (this->requires_serialization) {
                this->send(QUERY_ANSWER_TOKENS_FLOW_COMMAND, args, this->server_id);
            } else {
                this->send(QUERY_ANSWER_FLOW_COMMAND, args, this->server_id);
            }
            args.clear();
        }
        delete query_answer;
        Utils::sleep();
    }
}

template <class AnswerType>
QueryNodeClient<AnswerType>::QueryNodeClient(const string& node_id,
                                             const string& server_id,
                                             MessageBrokerType messaging_backend)
    : QueryNode<AnswerType>(node_id, true, messaging_backend) {
    this->query_answer_processor =
        new thread(&QueryNodeClient<AnswerType>::query_answer_processor_method, this);
    this->server_id = server_id;
    this->add_peer(server_id);
    this->join_network();
}

template <class AnswerType>
void QueryNodeClient<AnswerType>::node_joined_network(const string& node_id) {
    // do nothing
}

template <class AnswerType>
string QueryNodeClient<AnswerType>::cast_leadership_vote() {
    return this->server_id;
}

template <class AnswerType>
QueryAnswerFlow<AnswerType>::QueryAnswerFlow(string command, vector<string>& args) {
    for (auto pointer_string : args) {
        QueryAnswer* query_answer = (QueryAnswer*) stoul(pointer_string);
        this->query_answers.push_back(query_answer);
    }
}

template <class AnswerType>
void QueryAnswerFlow<AnswerType>::act(shared_ptr<MessageFactory> node) {
    auto query_node = dynamic_pointer_cast<QueryNodeServer<AnswerType>>(node);
    for (auto query_answer : this->query_answers) {
        query_node->add_query_answer(query_answer);
    }
}

template <class AnswerType>
QueryAnswerTokensFlow<AnswerType>::QueryAnswerTokensFlow(string command, vector<string>& args) {
    this->query_answers_tokens.reserve(args.size());
    for (auto token : args) {
        this->query_answers_tokens.push_back(token);
    }
}

template <class AnswerType>
void QueryAnswerTokensFlow<AnswerType>::act(shared_ptr<MessageFactory> node) {
    auto query_node = dynamic_pointer_cast<QueryNodeServer<AnswerType>>(node);
    for (auto tokens : this->query_answers_tokens) {
        AnswerType* query_answer = new AnswerType();
        query_answer->untokenize(tokens);
        query_node->add_query_answer(query_answer);
    }
}

template <class AnswerType>
QueryAnswersFinished<AnswerType>::QueryAnswersFinished(string command, vector<string>& args) {}

template <class AnswerType>
void QueryAnswersFinished<AnswerType>::act(shared_ptr<MessageFactory> node) {
    auto query_node = dynamic_pointer_cast<QueryNodeServer<AnswerType>>(node);
    query_node->query_answers_finished();
}
