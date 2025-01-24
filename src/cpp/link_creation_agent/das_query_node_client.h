#pragma once

#include <string>
#include <thread>

#include "star_node.h"
#include "queue.h"
#include "remote_iterator.h"

using namespace std;
using namespace distributed_algorithm_node;
using namespace atom_space_node;

namespace query_node {

/**
 * @brief Query Node class
 */
class QueryNode : public StarNode {
   public:
    QueryNode(const string& node_id, const string& server_id);
    ~QueryNode();
    shared_ptr<RemoteIterator> query(const vector<string>& tokens, const string& context);

   private:
    string PATTERN_MATCHING_QUERY = "pattern_matching_query";
    string next_query_id();
    string node_id;
    string server_id;
};

}  // namespace query_node
