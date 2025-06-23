#include "MorkDBAPITypes.h"

using namespace atomdb;
using namespace atomdb_api_types;

// --> HandlSetMork
HandleSetMork::HandleSetMork() : HandleSet(), handles_size(0) {}
HandleSetMork::HandleSetMork(std::string handle) : HandleSet() {
    this->handles.push_back(handle);
    this->handles_size += 1;
}
HandleSetMork::~HandleSetMork() {}
unsigned int HandleSetMork::size() {
    return this->handles_size;
}
void HandleSetMork::append(std::shared_ptr<HandleSet> other) {
    auto handle_set_mork = dynamic_pointer_cast<HandleSetMork>(other);
    for (auto handle : handle_set_mork->handles) {
        this->handles.push_back(handle);
        this->handles_size += 1;
    }
}
std::shared_ptr<HandleSetIterator> HandleSetMork::get_iterator() {
    shared_ptr<HandleSetMorkIterator> it(new HandleSetMorkIterator(this));
    return it;
}
// <--

// --> HandleSetIterator
HandleSetMorkIterator::HandleSetMorkIterator(HandleSetMork* handle_set) {
    this->handle_set = handle_set;
}
HandleSetMorkIterator::~HandleSetMorkIterator() {}
char* HandleSetMorkIterator::next() {
    if (this->handle_set->handles.empty()) {
        return nullptr;
    }

    auto it = this->handle_set->handles.begin();
    current = std::move(*it);
    this->handle_set->handles.erase(it);

    return &(current[0]);
}
// <--

// --> HandleList
// <--