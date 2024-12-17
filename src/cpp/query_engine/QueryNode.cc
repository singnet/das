#include "QueryNode.h"
#include "LeadershipBroker.h"
#include "MessageBroker.h"
#include "Utils.h"

using namespace query_node;
using namespace std;

string QueryNode::QUERY_ANSWER_TOKENS_FLOW_COMMAND = "query_answer_tokens_flow";
string QueryNode::QUERY_ANSWER_FLOW_COMMAND = "query_answer_flow";
string QueryNode::QUERY_ANSWERS_FINISHED_COMMAND = "query_answers_finished";

// --------------------------------------------------------------------------------
// Public methods

QueryNode::QueryNode(
    const string &node_id, 
    bool is_server,
    MessageBrokerType messaging_backend) :
    AtomSpaceNode(node_id, LeadershipBrokerType::SINGLE_MASTER_SERVER, messaging_backend) {

    this->is_server = is_server;
    this->query_answer_processor = NULL;
    this->query_answers_finished_flag = false;
    this->shutdown_flag = false;
    if (messaging_backend == MessageBrokerType::RAM) {
        this->requires_serialization = false;
    } else {
        this->requires_serialization = true;
    }
}

QueryNode::~QueryNode() {
}

void QueryNode::graceful_shutdown() {
    if (is_shutting_down()) {
        return;
    }
    AtomSpaceNode::graceful_shutdown();
    this->shutdown_flag_mutex.lock();
    this->shutdown_flag = true;
    this->shutdown_flag_mutex.unlock();
    if (this->query_answer_processor != NULL) {
        this->query_answer_processor->join();
        this->query_answer_processor = NULL;
    }
}

bool QueryNode::is_shutting_down() {
    bool answer;
    this->shutdown_flag_mutex.lock();
    answer = this->shutdown_flag;
    this->shutdown_flag_mutex.unlock();
    return answer;
}

void QueryNode::query_answers_finished() {
    this->query_answers_finished_flag_mutex.lock();
    this->query_answers_finished_flag = true;
    this->query_answers_finished_flag_mutex.unlock();
}

bool QueryNode::is_query_answers_finished() {
    bool answer;
    this->query_answers_finished_flag_mutex.lock();
    answer = this->query_answers_finished_flag;
    this->query_answers_finished_flag_mutex.unlock();
    return answer;
}

shared_ptr<Message> QueryNode::message_factory(string &command, vector<string> &args) {
    std::shared_ptr<Message> message = AtomSpaceNode::message_factory(command, args);
    if (message) {
        return message;
    }
    if (command == QueryNode::QUERY_ANSWER_FLOW_COMMAND) {
        return std::shared_ptr<Message>(new QueryAnswerFlow(command, args));
    } else if (command == QueryNode::QUERY_ANSWER_TOKENS_FLOW_COMMAND) {
        return std::shared_ptr<Message>(new QueryAnswerTokensFlow(command, args));
    } else if (command == QueryNode::QUERY_ANSWERS_FINISHED_COMMAND) {
        return std::shared_ptr<Message>(new QueryAnswersFinished(command, args));
    }
    return std::shared_ptr<Message>{};
}

void QueryNode::add_query_answer(QueryAnswer *query_answer) {
    if (is_query_answers_finished()) {
        Utils::error("Invalid addition of new query answer.");
    } else {
        this->query_answer_queue.enqueue((void *) query_answer);
    }
}

QueryAnswer *QueryNode::pop_query_answer() {
    return (QueryAnswer *) this->query_answer_queue.dequeue();
}

bool QueryNode::is_query_answers_empty() {
    return this->query_answer_queue.empty();
}

QueryNodeServer::QueryNodeServer(
    const string &node_id, 
    MessageBrokerType messaging_backend) : 
    QueryNode(node_id, true, messaging_backend) {

    this->join_network();
    this->query_answer_processor = new thread(
        &QueryNodeServer::query_answer_processor_method,
        this);
}

QueryNodeServer::~QueryNodeServer() {
    graceful_shutdown();
    if (this->query_answer_processor != NULL) {
        this->query_answer_processor->join();
        this->query_answer_processor = NULL;
    }
}

void QueryNodeServer::node_joined_network(const string &node_id) {
    this->add_peer(node_id);
}

string QueryNodeServer::cast_leadership_vote() {
    return this->node_id();
}

void QueryNodeServer::query_answer_processor_method() {
    while (! is_shutting_down()) {
        Utils::sleep();
    }
}

void QueryNodeClient::query_answer_processor_method() {
    QueryAnswer *query_answer;
    vector<string> args;
    bool answers_finished_flag = false;
    while (! is_shutting_down()) {
        while ((query_answer = (QueryAnswer *) this->query_answer_queue.dequeue()) != NULL) {
            if (this->requires_serialization) {
                string tokens = query_answer->tokenize();
                args.push_back(tokens);
            } else {
                args.push_back(to_string((unsigned long) query_answer));
            }
        }
        if (args.empty()) {
            // The order of the AND clauses below matters
            if (! answers_finished_flag && is_query_answers_finished() && this->query_answer_queue.empty()) {
                this->send(QueryNode::QUERY_ANSWERS_FINISHED_COMMAND, args, this->server_id);
                answers_finished_flag = true;
            }
        } else {
            if (this->requires_serialization) {
                this->send(QueryNode::QUERY_ANSWER_TOKENS_FLOW_COMMAND, args, this->server_id);
            } else {
                this->send(QueryNode::QUERY_ANSWER_FLOW_COMMAND, args, this->server_id);
            }
            args.clear();
        }
        Utils::sleep();
    }
}

QueryNodeClient::QueryNodeClient(
    const string &node_id, 
    const string &server_id,
    MessageBrokerType messaging_backend) : 
    QueryNode(node_id, true, messaging_backend) {

    this->query_answer_processor = new thread(
        &QueryNodeClient::query_answer_processor_method,
        this);
    this->server_id = server_id;
    this->add_peer(server_id);
    this->join_network();
}

QueryNodeClient::~QueryNodeClient() {
    graceful_shutdown();
    if (this->query_answer_processor != NULL) {
        this->query_answer_processor->join();
        this->query_answer_processor = NULL;
    }
}

void QueryNodeClient::node_joined_network(const string &node_id) {
    // do nothing
}

string QueryNodeClient::cast_leadership_vote() {
    return this->server_id;
}

QueryAnswerFlow::QueryAnswerFlow(string command, vector<string> &args) {
    for (auto pointer_string: args) {
        QueryAnswer *query_answer = (QueryAnswer *) stoul(pointer_string);
        this->query_answers.push_back(query_answer);
    }
}

void QueryAnswerFlow::act(shared_ptr<MessageFactory> node) {
    auto query_node = dynamic_pointer_cast<QueryNodeServer>(node);
    for (auto query_answer: this->query_answers) {
        query_node->add_query_answer(query_answer);
    }
}

QueryAnswerTokensFlow::QueryAnswerTokensFlow(string command, vector<string> &args) {
    for (auto tokens: args) {
        this->query_answers_tokens.push_back(tokens);
    }
}

void QueryAnswerTokensFlow::act(shared_ptr<MessageFactory> node) {
    auto query_node = dynamic_pointer_cast<QueryNodeServer>(node);
    for (auto tokens: this->query_answers_tokens) {
        QueryAnswer *query_answer = new QueryAnswer();
        query_answer->untokenize(tokens);
        query_node->add_query_answer(query_answer);
    }
}

QueryAnswersFinished::QueryAnswersFinished(string command, vector<string> &args) {
}

void QueryAnswersFinished::act(shared_ptr<MessageFactory> node) {
    auto query_node = dynamic_pointer_cast<QueryNodeServer>(node);
    query_node->query_answers_finished();
}
