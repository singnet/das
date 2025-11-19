#include <string>

#include "AtomPropertiesIndex.h"
#include "Hasher.h"

using namespace commons;
using namespace std;

AtomProperties::AtomProperties(const vector<string>& composite_type, const string& metta_expression) {
    this->composite_type = composite_type;
    this->composite_type_hash = Hasher::composite_handle(composite_type);
    this->metta_expression = metta_expression;
}

void AtomProperties::merge(TrieValue* other) {
    return;
}

AtomPropertiesIndex::AtomPropertiesIndex() : trie(HANDLE_HASH_SIZE - 1) {}

void AtomPropertiesIndex::set(const string& handle, AtomProperties* atom_properties) {
    this->trie.insert(handle, atom_properties);
}

AtomProperties* AtomPropertiesIndex::get(const string& handle) {
    return static_cast<AtomProperties*>(this->trie.lookup(handle));
}
