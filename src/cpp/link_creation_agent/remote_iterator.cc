#include "remote_iterator.h"

using namespace query_node;

RemoteIterator::RemoteIterator(const string& local_id) : local_id(local_id) {
    remote_iterator_node = new QueryNodeServer(local_id);
}

RemoteIterator::~RemoteIterator() {}

