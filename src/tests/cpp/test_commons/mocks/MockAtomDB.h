#pragma once
#include "AtomDB.h"
#include "AtomDBAPITypes.h"
#include "gmock/gmock.h"

using namespace std;
using namespace atomdb;

class MockAtomDocument : public atomdb_api_types::AtomDocument {
   public:
    MOCK_METHOD(const char*, get, (const string& key), (override));
    MOCK_METHOD(const char*, get, (const string& array_key, unsigned int index), (override));
    MOCK_METHOD(bool, get_bool, (const string& key), (override));
    MOCK_METHOD(unsigned int, get_size, (const string& array_key), (override));
    MOCK_METHOD(bool, contains, (const string& key), (override));
    MockAtomDocument() {
        ON_CALL(*this, get(testing::_)).WillByDefault(::testing::Return("mocked_value"));
        testing::Mock::AllowLeak(this);
        ON_CALL(*this, contains).WillByDefault(::testing::Return(true));
        ON_CALL(*this, get_size).WillByDefault(::testing::Return(1));
    }
};

class AtomDBMock : public AtomDB {
   public:
    MOCK_METHOD(bool, allow_nested_indexing, (), (override));
    MOCK_METHOD(shared_ptr<Atom>, get_atom, (const string& handle), (override));
    MOCK_METHOD(shared_ptr<atomdb_api_types::HandleSet>,
                query_for_pattern,
                (const LinkSchema& link_template),
                (override));
    MOCK_METHOD(shared_ptr<atomdb_api_types::HandleList>,
                query_for_targets,
                (const string& handle),
                (override));

    MOCK_METHOD(shared_ptr<atomdb_api_types::HandleSet>,
                query_for_incoming_set,
                (const string& handle),
                (override));

    MOCK_METHOD(shared_ptr<atomdb_api_types::AtomDocument>,
                get_atom_document,
                (const string& handle),
                (override));
    MOCK_METHOD(shared_ptr<atomdb_api_types::AtomDocument>,
                get_node_document,
                (const string& handle),
                (override));
    MOCK_METHOD(shared_ptr<atomdb_api_types::AtomDocument>,
                get_link_document,
                (const string& handle),
                (override));

    MOCK_METHOD(vector<shared_ptr<atomdb_api_types::AtomDocument>>,
                get_atom_documents,
                (const vector<string>& handles, const vector<string>& fields),
                (override));
    MOCK_METHOD(vector<shared_ptr<atomdb_api_types::AtomDocument>>,
                get_node_documents,
                (const vector<string>& handles, const vector<string>& fields),
                (override));
    MOCK_METHOD(vector<shared_ptr<atomdb_api_types::AtomDocument>>,
                get_link_documents,
                (const vector<string>& handles, const vector<string>& fields),
                (override));

    MOCK_METHOD(vector<shared_ptr<atomdb_api_types::AtomDocument>>,
                get_matching_atoms,
                (bool is_toplevel, Atom& key),
                (override));

    MOCK_METHOD(bool, atom_exists, (const string& handle), (override));
    MOCK_METHOD(bool, node_exists, (const string& handle), (override));
    MOCK_METHOD(bool, link_exists, (const string& handle), (override));

    MOCK_METHOD(set<string>, atoms_exist, (const vector<string>& handles), (override));
    MOCK_METHOD(set<string>, nodes_exist, (const vector<string>& handles), (override));
    MOCK_METHOD(set<string>, links_exist, (const vector<string>& link_handles), (override));

    MOCK_METHOD(string, add_node, (const Node* node, bool throw_if_exists), (override));
    MOCK_METHOD(string, add_link, (const Link* link, bool throw_if_exists), (override));
    MOCK_METHOD(string, add_atom, (const Atom* atom, bool throw_if_exists), (override));

    MOCK_METHOD(vector<string>,
                add_atoms,
                (const vector<Atom*>& atoms, bool throw_if_exists, bool is_transactional),
                (override));
    MOCK_METHOD(vector<string>,
                add_nodes,
                (const vector<Node*>& nodes, bool throw_if_exists, bool is_transactional),
                (override));
    MOCK_METHOD(vector<string>,
                add_links,
                (const vector<Link*>& links, bool throw_if_exists, bool is_transactional),
                (override));

    MOCK_METHOD(bool, delete_atom, (const string& handle, bool delete_link_targets), (override));
    MOCK_METHOD(bool, delete_node, (const string& handle, bool delete_link_targets), (override));
    MOCK_METHOD(bool, delete_link, (const string& handle, bool delete_link_targets), (override));

    MOCK_METHOD(uint,
                delete_atoms,
                (const vector<string>& handles, bool delete_link_targets),
                (override));
    MOCK_METHOD(uint,
                delete_nodes,
                (const vector<string>& handles, bool delete_link_targets),
                (override));
    MOCK_METHOD(uint,
                delete_links,
                (const vector<string>& handles, bool delete_link_targets),
                (override));

    MOCK_METHOD(void, re_index_patterns, (bool flush_patterns), (override));

    AtomDBMock() {
        ON_CALL(*this, get_atom_document(testing::_))
            .WillByDefault(::testing::Return(make_shared<MockAtomDocument>()));
    }
};
