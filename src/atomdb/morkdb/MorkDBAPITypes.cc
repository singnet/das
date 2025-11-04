#include "MorkDBAPITypes.h"

#include <cstring>

#include "RedisMongoDB.h"

using namespace atomdb;
using namespace atomdb_api_types;

// --> HandlSetMork
HandleSetMork::HandleSetMork() : HandleSet(), handles_size(0) {}

HandleSetMork::HandleSetMork(string handle) : HandleSet() {
    this->handles.insert(handle);
    this->handles_size += 1;
}

HandleSetMork::HandleSetMork(string handle,
                             map<string, string> metta_expressions,
                             Assignment assignments)
    : HandleSet() {
    this->handles.insert(handle);
    this->metta_expressions_by_handle[handle] = metta_expressions;
    this->assignments_by_handle[handle] = assignments;
    this->handles_size += 1;
}

HandleSetMork::~HandleSetMork() {}

unsigned int HandleSetMork::size() { return this->handles_size; }

void HandleSetMork::append(shared_ptr<HandleSet> other) {
    auto handle_set_mork = dynamic_pointer_cast<HandleSetMork>(other);
    for (auto handle : handle_set_mork->handles) {
        this->handles.insert(handle);
        this->metta_expressions_by_handle[handle] = handle_set_mork->metta_expressions_by_handle[handle];
        this->assignments_by_handle[handle] = handle_set_mork->assignments_by_handle[handle];
        this->handles_size += 1;
    }
}

shared_ptr<HandleSetIterator> HandleSetMork::get_iterator() {
    return make_shared<HandleSetMorkIterator>(this);
}
// <--

// --> HandleSetMorkIterator
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

map<string, string> HandleSetMork::get_metta_expressions_by_handle(const string& handle) {
    return this->metta_expressions_by_handle[handle];
}

Assignment HandleSetMork::get_assignments_by_handle(const string& handle) {
    return this->assignments_by_handle[handle];
}

// <--

// --> HandleListMork
HandleListMork::HandleListMork(const shared_ptr<AtomDocument>& document) : HandleList() {
    unsigned int handles_size = 0;
    if (document->contains(RedisMongoDB::MONGODB_FIELD_NAME[MONGODB_FIELD::TARGETS])) {
        handles_size = document->get_size(RedisMongoDB::MONGODB_FIELD_NAME[MONGODB_FIELD::TARGETS]);
    }
    this->handles_size = handles_size;
    this->handles = new char*[handles_size];
    for (unsigned int i = 0; i < handles_size; i++) {
        this->handles[i] =
            strdup(document->get(RedisMongoDB::MONGODB_FIELD_NAME[MONGODB_FIELD::TARGETS], i));
    }
}

HandleListMork::~HandleListMork() {
    for (unsigned int i = 0; i < this->handles_size; i++) {
        free(this->handles[i]);
    }
    delete[] this->handles;
}

const char* HandleListMork::get_handle(unsigned int index) {
    if (index > this->handles_size) {
        Utils::error("Handle index out of bounds: " + to_string(index) +
                     " Answer handles size: " + to_string(this->handles_size));
    }
    return handles[index];
}

unsigned int HandleListMork::size() { return this->handles_size; }
// <--