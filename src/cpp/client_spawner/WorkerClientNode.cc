#include "WorkerClientNode.h"
#include "SentinelServerNode.h"
#include "DASNode.h"
#include "RemoteIterator.h"
#include "QueryAnswer.h"

using namespace client_spawner;
using namespace query_engine;
using namespace query_element;

#define MAX_QUERY_ANSWERS ((unsigned int) 1000)

WorkerClientNode::WorkerClientNode(
    const string &node_id,
    const string &server_id
) : StarNode(node_id, server_id) {}

WorkerClientNode::~WorkerClientNode() {}

void WorkerClientNode::execute(const vector<string> &request, const string &das_node_server_id) {
    DASNode das_node_client(this->node_id(), das_node_server_id);
    
    QueryAnswer *query_answer;
    
    unsigned int count = 0;
    
    RemoteIterator *response = das_node_client.pattern_matcher_query(request);

    vector<string> message;
    
    message.push_back(this->node_id());
    
    string query_answer_str;

    while (! response->finished()) {
        if ((query_answer = response->pop()) == NULL) {
            Utils::sleep();
        } else {
            query_answer_str += query_answer->to_string();
            query_answer_str += "\n";
            if (++count == MAX_QUERY_ANSWERS) {
                break;
            }
        }
    }
    
    if (count == 0) {
        string no_match = "No match for query";
#ifdef DEBUG
        cout << no_match << endl;
#endif
        message.push_back(no_match);
    } else {
#ifdef DEBUG
        cout << query_answer_str << endl;
#endif
        message.push_back(query_answer_str);
    }

    delete response;

    send(WORKER_NOTIFICATION, message, this->server_id);

}
