#include "das_query_node_client.h"

using namespace query_node;
using namespace distributed_algorithm_node;
using namespace query_node;
using namespace std;



QueryNode::QueryNode(
    const string &node_id, 
    const string &server_id): StarNode(node_id, server_id) {
        this->node_id = node_id;
        this->server_id = server_id;
}


QueryNode::~QueryNode() {
}

shared_ptr<RemoteIterator> QueryNode::query(const vector<string> &tokens, const string &context) {
    string query_id = next_query_id();
    vector<string> args = {query_id, context};
    args.insert(args.end(), tokens.begin(), tokens.end());
    send(PATTERN_MATCHING_QUERY, args, this->server_id);
    return make_shared<RemoteIterator>(query_id);
}


string QueryNode::next_query_id() {
    string host = this->node_id.substr(0, node_id.find(":"));
    int port = stoi(this->node_id.substr(node_id.find(":") + 1));
    return host + ":" + to_string(++port);
}