#pragma once

#include <string>
#include <vector>
#include "HandleTrie.h"

using namespace std;
using namespace commons;

class AtomProperties : public HandleTrie::TrieValue {
   public:
    AtomProperties(const vector<string>& composite_type, const string& metta_expression);
    void merge(TrieValue* other);

   public:
    const vector<string>& get_composite_type() const { return this->composite_type; }
    const string& get_composite_type_hash() const { return this->composite_type_hash; }
    const string& get_metta_expression() const { return this->metta_expression; }

   private:
    vector<string> composite_type;
    string composite_type_hash;
    string metta_expression;
};

class AtomPropertiesIndex {
   public:
    AtomPropertiesIndex();
    void set(const string& handle, AtomProperties* atom_properties);
    AtomProperties* get(const string& handle);

   private:
    HandleTrie trie;
};
