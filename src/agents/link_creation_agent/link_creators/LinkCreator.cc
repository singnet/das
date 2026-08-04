#include "LinkCreator.h"

using namespace link_creators;

// -------------------------------------------------------------------------------------------------
// Public methods

LinkCreator::LinkCreator() {
    this->_context = "";
    this->_visited_at_least_one_in_last_create = false;
}

LinkCreator::~LinkCreator() {}

bool LinkCreator::get_and_reset_visited() {
    bool answer = _visited_at_least_one_in_last_create;
    this->_visited_at_least_one_in_last_create = false;
    return answer;
}

void LinkCreator::visit(const string& key) {
    this->_visited.insert(key);
    this->_visited_at_least_one_in_last_create = true;
}

bool LinkCreator::visited(const string& key) {
    return (this->_visited.find(key) != this->_visited.end());
}
