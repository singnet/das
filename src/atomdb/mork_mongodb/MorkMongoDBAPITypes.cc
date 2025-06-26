#include "MorkMongoDBAPITypes.h"

using namespace atomdb;
using namespace atomdb_api_types;

// --> HandlSetMork
HandleSetMork::HandleSetMork() : HandleSet(), handles_size(0) {}
HandleSetMork::HandleSetMork(string handle) : HandleSet() {
    this->handles.insert(handle);
    this->handles_size += 1;
}
HandleSetMork::~HandleSetMork() {}
unsigned int HandleSetMork::size() { return this->handles_size; }
void HandleSetMork::append(shared_ptr<HandleSet> other) {
    auto handle_set_mork = dynamic_pointer_cast<HandleSetMork>(other);
    for (auto handle : handle_set_mork->handles) {
        this->handles.insert(handle);
        this->handles_size += 1;
    }
}
shared_ptr<HandleSetIterator> HandleSetMork::get_iterator() {
    return make_shared<HandleSetMorkIterator>(this);
}
// <--

// --> HandleSetIterator
HandleSetMorkIterator::HandleSetMorkIterator(HandleSetMork* handle_set) : handle_set(handle_set) {
    it = handle_set->handles.begin();
    end = handle_set->handles.end();
}
HandleSetMorkIterator::~HandleSetMorkIterator() {}
char* HandleSetMorkIterator::next() {
    if (it == end) return nullptr;
    const string& s = *it;
    ++it;
    return const_cast<char*>(s.c_str());
}
// <--

// --> HandleList
// <--