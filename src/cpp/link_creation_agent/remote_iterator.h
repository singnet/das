#pragma once

#include <string>

#include "star_node.h"
#include "das_query_node_server.h"

using namespace std;
using namespace atom_space_node;

namespace query_node {


class RemoteIterator {
   public:
    RemoteIterator(const string& local_id);
    ~RemoteIterator();

   private:
    string local_id;
    QueryNodeServer* remote_iterator_node;

};

}  // namespace query_node
