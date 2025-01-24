#include "das_query_node_server.h"

using namespace query_node;

QueryNodeServer::QueryNodeServer(const string& node_id) : StarNode(node_id) {}

QueryNodeServer::~QueryNodeServer() {
    shutting_down = true;
    DistributedAlgorithmNode::graceful_shutdown();
}

void QueryNodeServer::add_request(string& request) { request_queue.enqueue(request); }

string QueryNodeServer::pop_request() { return request_queue.dequeue(); }

bool QueryNodeServer::is_query_empty() { return request_queue.empty(); }

bool QueryNodeServer::is_shutting_down() { return shutting_down; }

shared_ptr<Message> QueryNodeServer::message_factory(string& command, vector<string>& args) {
    cout << "QueryNodeServer::message_factory" << endl;
    cout << command << endl;
    shared_ptr<Message> message = StarNode::message_factory(command, args);
    if (message) {
        return message;
    }
    if (command == QUERY_ANSWER_TOKENS_FLOW_COMMAND) {
        return make_shared<QueryAnswerTokensFlow>(command, args);
    }
    if (command == QUERY_ANSWER_FLOW_COMMAND) {
        return make_shared<QueryAnswerFlow>(command, args);
    }
    if (command == QUERY_ANSWERS_FINISHED_COMMAND) {
        return make_shared<QueryAnswersFinished>(command, args);
    }
    cout << "Command not recognized" << endl;
    return make_shared<DummyMessage>(command, args);
}

QueryAnswerTokensFlow::QueryAnswerTokensFlow(string command, vector<string>& args) {
    this->command = command;
    this->args = args;
}

void QueryAnswerTokensFlow::act(shared_ptr<MessageFactory> node) {
    cout << "QueryAnswerTokensFlow::act" << endl;
    auto server_node = dynamic_pointer_cast<QueryNodeServer>(node);
    string request;
    for (auto arg : this->args)
    {
        request += arg + " ";
    }
    server_node->add_request(request);
}

QueryAnswerFlow::QueryAnswerFlow(string command, vector<string>& args) {
    this->command = command;
    this->args = args;
}

void QueryAnswerFlow::act(shared_ptr<MessageFactory> node) {
    cout << "QueryAnswerFlow::act" << endl;
}

QueryAnswersFinished::QueryAnswersFinished(string command, vector<string>& args) {
    this->command = command;
    this->args = args;
}

void QueryAnswersFinished::act(shared_ptr<MessageFactory> node) {
    cout << "QueryAnswersFinished::act" << endl;

}
