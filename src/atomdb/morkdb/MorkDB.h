#pragma once

#include <httplib.h>

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/json.hpp>
#include <memory>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/options/find.hpp>
#include <mongocxx/pool.hpp>
#include <mutex>
#include <vector>

#include "AtomDB.h"
#include "AtomDBCacheSingleton.h"
#include "MorkDBAPITypes.h"
#include "RedisMongoDB.h"

namespace atomdb {

class MorkClient {
   public:
    MorkClient(const string& base_url = "");
    ~MorkClient();
    string post(const string& data, const string& pattern = "$x", const string& template_ = "$x");
    vector<string> get(const string& pattern, const string& template_);

   private:
    string base_url_;
    httplib::Client cli;
    httplib::Result send_request(const string& method, const string& path, const string& data = "");
    string url_encode(const string& value);
};

class MorkDB : public AtomDB {
   public:
    MorkDB(const string& context = "");
    ~MorkDB();

    bool allow_nested_indexing() override;

    shared_ptr<Atom> get_atom(const string& handle) override;  // HandleDecoder interface

    shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(const LinkSchema& link_schema) override;
    shared_ptr<atomdb_api_types::HandleList> query_for_targets(const string& handle) override;
    shared_ptr<atomdb_api_types::HandleSet> query_for_incoming_set(const string& handle) override;

    shared_ptr<atomdb_api_types::AtomDocument> get_atom_document(const string& handle) override;
    shared_ptr<atomdb_api_types::AtomDocument> get_node_document(const string& handle) override;
    shared_ptr<atomdb_api_types::AtomDocument> get_link_document(const string& handle) override;

    vector<shared_ptr<atomdb_api_types::AtomDocument>> get_atom_documents(
        const vector<string>& handles, const vector<string>& fields) override;
    vector<shared_ptr<atomdb_api_types::AtomDocument>> get_node_documents(
        const vector<string>& handles, const vector<string>& fields) override;
    vector<shared_ptr<atomdb_api_types::AtomDocument>> get_link_documents(
        const vector<string>& handles, const vector<string>& fields) override;

    vector<shared_ptr<atomdb_api_types::AtomDocument>> get_matching_atoms(bool is_toplevel,
                                                                          Atom& key) override;

    bool atom_exists(const string& handle) override;
    bool node_exists(const string& handle) override;
    bool link_exists(const string& handle) override;

    set<string> atoms_exist(const vector<string>& handles) override;
    set<string> nodes_exist(const vector<string>& handles) override;
    set<string> links_exist(const vector<string>& handles) override;

    string add_atom(const atoms::Atom* atom) override;
    string add_node(const atoms::Node* node) override;
    string add_link(const atoms::Link* link) override;

    vector<string> add_atoms(const vector<atoms::Atom*>& atoms) override;
    vector<string> add_nodes(const vector<atoms::Node*>& nodes) override;
    vector<string> add_links(const vector<atoms::Link*>& links) override;

    bool delete_atom(const string& handle, bool delete_link_targets = false) override;
    bool delete_node(const string& handle, bool delete_link_targets = false) override;
    bool delete_link(const string& handle, bool delete_link_targets = false) override;

    uint delete_atoms(const vector<string>& handles, bool delete_link_targets = false) override;
    uint delete_nodes(const vector<string>& handles, bool delete_link_targets = false) override;
    uint delete_links(const vector<string>& handles, bool delete_link_targets = false) override;

    // Used for testing purposes
    shared_ptr<atomdb_api_types::HandleSet> query_for_pattern_base(const LinkSchema& link_schema);

   private:
    shared_ptr<RedisMongoDB> mongodb;
    shared_ptr<AtomDBCache> atomdb_cache;
    shared_ptr<MorkClient> mork_client;

    void mork_setup();
};

}  // namespace atomdb
