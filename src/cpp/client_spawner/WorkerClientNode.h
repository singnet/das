/**
 * @file worker_client_node.h
 * @brief Responsible for making a query
*/
#pragma once

#include "StarNode.h"

using namespace distributed_algorithm_node;

namespace client_spawner {

class WorkerClientNode : public StarNode {

public:
    WorkerClientNode(const string& node_id, const string& server_id, const string& das_node_server_id);
    
    ~WorkerClientNode();
    
    void execute(vector<string>& request);

private:
    string node_id;
    string das_node_server_id;
};

}  // namespace client_spawner
