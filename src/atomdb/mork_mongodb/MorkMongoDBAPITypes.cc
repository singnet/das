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

MongodbDocument2::MongodbDocument2(
    bsoncxx::v_noabi::stdx::optional<bsoncxx::v_noabi::document::value>& document) {
    this->document = document;
}

MongodbDocument2::~MongodbDocument2() {}

const char* MongodbDocument2::get(const string& key) {
    // Note for reference: .to_string() instead of .data() would return a std::string
    return ((*this->document)[key]).get_string().value.data();
}

const char* MongodbDocument2::get(const string& array_key, unsigned int index) {
    // Note for reference: .to_string() instead of .data() would return a std::string
    return ((*this->document)[array_key]).get_array().value[index].get_string().value.data();
}

bool MongodbDocument2::contains(const string& key) {
    return ((*this->document).view()).find(key) != ((*this->document).view()).end();
}

unsigned int MongodbDocument2::get_size(const string& array_key) {
    // NOTE TO REVIEWER
    // TODO: this implementation is wrong and need to be fixed before integration in das-atom-db
    // I couldn't figure out a way to discover the number of elements in a BSON array.
    // cout << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" << endl;
    // cout << "XXXXXXXXXXXXXXXXXXX MongodbDocument::get_size()" << endl;
    // cout << "XXXXXXXXXXXXXXXXXXX MongodbDocument::get_size() length: " <<
    // ((*this->document)[array_key]).get_array().value.length() << endl; cout << "XXXXXXXXXXXXXXXXXXX
    // MongodbDocument::get_size() HASH: " << HANDLE_HASH_SIZE << endl; cout << "XXXXXXXXXXXXXXXXXXX
    // MongodbDocument::get_size() size: " << ((*this->document)[array_key]).get_array().value.length() /
    // HANDLE_HASH_SIZE << endl;
    return ((*this->document)[array_key]).get_array().value.length() / HANDLE_HASH_SIZE;
}