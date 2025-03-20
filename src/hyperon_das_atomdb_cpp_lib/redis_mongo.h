/**
 * @file redis_mongo.h
 */
#pragma once

#include <hiredis_cluster/hircluster.h>

#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>
#include <mutex>
#include <optional>

#include "database.h"
#include "document_types.h"
#include "type_aliases.h"

using namespace std;

namespace atomdb {

class RedisMongoDB : public AtomDB {
   public:
    RedisMongoDB(const string& database_name = "das",
                 const string& mongodb_host = "localhost",
                 const string& mongo_port = "27017",
                 const string& mongo_user = "",
                 const string& mongo_password = "",
                 const string& mongo_tls_ca_file = "",
                 const string& redis_host = "localhost",
                 int redis_port = 6379,
                 const string& redis_user = "",
                 const string& redis_password = "",
                 bool redis_cluster = false bool redis_ssl = false);
    ~RedisMongoDB();

    const string get_node_handle(const string& node_type, const string& node_name) const override;

    const string get_node_name(const string& node_handle) const override;

    const string get_node_type(const string& node_handle) const override;

    const StringList get_node_by_name(const string& node_type, const string& substring) const override;

    const StringList get_atoms_by_field(
        const vector<unordered_map<string, string>>& query) const override;

    const pair<const int, const AtomList> get_atoms_by_index(const string& index_id,
                                                             const vector<map<string, string>>& query,
                                                             int cursor = 0,
                                                             int chunk_size = 500) const override;

    const StringList get_atoms_by_text_field(
        const string& text_value,
        const optional<string>& field = nullopt,
        const optional<string>& text_index_id = nullopt) const override;

    const StringList get_node_by_name_starting_with(const string& node_type,
                                                    const string& startswith) const override;

    const StringList get_all_nodes_handles(const string& node_type) const override;

    const StringList get_all_nodes_names(const string& node_type) const override;

    const StringUnorderedSet get_all_links(const string& link_type) const override;

    const string get_link_handle(const string& link_type,
                                 const StringList& target_handles) const override;

    const string get_link_type(const string& link_handle) const override;

    const StringList get_link_targets(const string& link_handle) const override;

    const StringList get_incoming_links_handles(const string& atom_handle,
                                                const KwArgs& kwargs = {}) const override;

    const vector<shared_ptr<const Atom>> get_incoming_links_atoms(
        const string& atom_handle, const KwArgs& kwargs = {}) const override;

    const StringUnorderedSet get_matched_links(const string& link_type,
                                               const StringList& target_handles,
                                               const KwArgs& kwargs = {}) const override;

    const StringUnorderedSet get_matched_type_template(const StringList& _template,
                                                       const KwArgs& kwargs = {}) const override;

    const StringUnorderedSet get_matched_type(const string& link_type,
                                              const KwArgs& kwargs = {}) const override;

    const optional<const string> get_atom_type(const string& handle) const override;

    const unordered_map<string, int> count_atoms() const override;

    void clear_database() override;

    const shared_ptr<const Node> add_node(const Node& node_params) override;

    const shared_ptr<const Link> add_link(const Link& link_params, bool toplevel = true) override;

    void reindex(const unordered_map<string, vector<unordered_map<string, any>>>&
                     pattern_index_templates) override;

    void delete_atom(const string& handle) override;

    const string create_field_index(const string& atom_type,
                                    const StringList& fields,
                                    const string& named_type = "",
                                    const optional<const StringList>& composite_type = nullopt,
                                    FieldIndexType index_type = FieldIndexType::BINARY_TREE) override;

    void bulk_insert(const vector<shared_ptr<const Atom>>& documents) override;

    const vector<shared_ptr<const Atom>> retrieve_all_atoms() const override;

    void commit(const optional<const vector<Atom>>& buffer = nullopt) override;

   protected:
    string database_name;
    redisClusterContext* redis_cluster;
    redisContext* redis_single;
    mongocxx::client* mongodb_client;
    mongocxx::database mongodb;
    mongocxx::v_noabi::collection mongodb_collection;
    mutex mongodb_mutex;
    mongocxx::pool* mongodb_pool;
    


    const string& database_name;
    const string& mongodb_host;
    const string& mongo_port;
    const string& mongo_user;
    const string& mongo_password;
    const string& mongo_tls_ca_file;
    const string& redis_host;
    const string& collection_name;
    const string& wildcard;
    const string& redis_patterns_prefix;
    const string& redis_targets_prefix;
    int redis_port;
    const string& redis_user;
    const string& redis_password;
    bool redis_cluster;
    bool redis_ssl;
    int max_buffer_size = 1000;

   private:
    void _setup_mongodb();
    void _setup_redis();
    void _setup_indexes();
    void _update_atom_indexes(const optional<const vector<Atom>>& buffer = nullopt);
    bsoncxx::document _build_node_document(const Node& node);
    bsoncxx::document _build_link_document(const Link& link);
    std::vector<bsoncxx::document> documents_buffer;

    // shared_ptr<Atom> _get_atom(const char *handle);
    // vector<string> _build_named_type_template(const vector<string>& named_type_template);
    // vector<string> _get_document_keys(shared_ptr<Atom> document);
    // vector<string> _filter_non_toplevel(const vector<string>& handles);
    // shared_ptr<Link> _build_link(Link& link_params, bool toplevel);
    // shared_ptr<Node> _build_node(Node& node_params);
};

}  // namespace atomdb