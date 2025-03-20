#include "redis_mongo.h"

#include <iostream>
#include <stdexcept>

using namespace std;
using namespace atomdb;

RedisMongoDB::RedisMongoDB(const string& database_name,
                           const string& mongodb_host,
                           const string& mongo_port,
                           const string& mongo_user,
                           const string& mongo_password,
                           const string& mongo_tls_ca_file,
                           const string& redis_host,
                           int redis_port,
                           const string& redis_user,
                           const string& redis_password,
                           bool redis_cluster,
                           bool redis_ssl) {
    this->database_name = database_name;
    this->mongodb_host = mongodb_host;
    this->mongo_port = mongo_port;
    this->mongo_user = mongo_user;
    this->mongo_password = mongo_password;
    this->mongo_tls_ca_file = mongo_tls_ca_file;
    this->redis_host = redis_host;
    this->redis_port = redis_port;
    this->redis_user = redis_user;
    this->redis_password = redis_password;
    this->redis_cluster = redis_cluster;
    this->redis_ssl = redis_ssl;
    this->collection_name = "atoms" this->wildcard = "*";
    this->redis_patterns_prefix = "patterns";
    this->redis_targets_prefix = "outgoing_set";
    _setup_mongodb();
    _setup_redis();
}

RedisMongoDB::~RedisMongoDB() {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

const string RedisMongoDB::get_node_handle(const string& node_type, const string& node_name) const {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

const string RedisMongoDB::get_node_name(const string& node_handle) const {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

const string RedisMongoDB::get_node_type(const string& node_handle) const {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

const StringList RedisMongoDB::get_node_by_name(const string& node_type, const string& substring) const {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

const StringList RedisMongoDB::get_atoms_by_field(
    const vector<unordered_map<string, string>>& query) const {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

const pair<const int, const AtomList> RedisMongoDB::get_atoms_by_index(
    const string& index_id, const vector<map<string, string>>& query, int cursor, int chunk_size) const {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

const StringList RedisMongoDB::get_atoms_by_text_field(const string& text_value,
                                                       const optional<string>& field,
                                                       const optional<string>& text_index_id) const {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

const StringList RedisMongoDB::get_node_by_name_starting_with(const string& node_type,
                                                              const string& startswith) const {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

const StringList RedisMongoDB::get_all_nodes_handles(const string& node_type) const {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

const StringList RedisMongoDB::get_all_nodes_names(const string& node_type) const {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

const StringUnorderedSet RedisMongoDB::get_all_links(const string& link_type) const {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

const string RedisMongoDB::get_link_handle(const string& link_type,
                                           const StringList& target_handles) const {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

const string RedisMongoDB::get_link_type(const string& link_handle) const {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

const StringList RedisMongoDB::get_link_targets(const string& link_handle) const {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

const StringList RedisMongoDB::get_incoming_links_handles(const string& atom_handle,
                                                          const KwArgs& kwargs) const {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

const vector<shared_ptr<const Atom>> RedisMongoDB::get_incoming_links_atoms(const string& atom_handle,
                                                                            const KwArgs& kwargs) const {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

const StringUnorderedSet RedisMongoDB::get_matched_links(const string& link_type,
                                                         const StringList& target_handles,
                                                         const KwArgs& kwargs) const {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

const StringUnorderedSet RedisMongoDB::get_matched_type_template(const StringList& _template,
                                                                 const KwArgs& kwargs) const {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

const StringUnorderedSet RedisMongoDB::get_matched_type(const string& link_type,
                                                        const KwArgs& kwargs) const {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

const optional<const string> RedisMongoDB::get_atom_type(const string& handle) const {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

const unordered_map<string, int> RedisMongoDB::count_atoms() const {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

void RedisMongoDB::clear_database() {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

const shared_ptr<const Node> RedisMongoDB::add_node(const Node& node_params) {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

const shared_ptr<const Link> RedisMongoDB::add_link(const Link& link_params, bool toplevel) {
    shared_ptr<const Link> link = this->_build_link(link_params);
    if (link == nullptr) {
        return nullptr;
    }
    try {
        this->mongodb_mutex.lock();
        auto document = this->_build_link_document(link);
        this->documents_buffer.push_back(document);
        if (this->documents_buffer.size() >= this->buffer_size) {
            this->commit();
        }
        this->mongodb_mutex.unlock();
        return link;
    } catch (const std::exception& e) {
        runtime_error(e.what());
    }
}

void RedisMongoDB::reindex(
    const unordered_map<string, vector<unordered_map<string, any>>>& pattern_index_template) {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

void RedisMongoDB::delete_atom(const string& handle) {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

const string RedisMongoDB::create_field_index(const string& atom_type,
                                              const StringList& fields,
                                              const string& named_type,
                                              const optional<const StringList>& composite_type,
                                              FieldIndexType index_type) {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

void RedisMongoDB::bulk_insert(const vector<shared_ptr<const Atom>>& documents) {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

const vector<shared_ptr<const Atom>> RedisMongoDB::retrieve_all_atoms() const {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

void RedisMongoDB::commit(const optional<const vector<Atom>>& buffer) {
    if (buffer != nullopt) {
        // TODO Implement
        throw std::runtime_error("Not implemented");
    }
    this->mongodb_mutex.lock();
    for (const auto& document : this->documents_buffer) {
        auto replace_options = mongocxx::options::replace();
        replace_options.upsert(true);
        this->mongodb_collection.replace_one(document.view(), replace_options);
    }
    this->_update_atom_indexes(buffer);
    this->documents_buffer.clear();
    this->mongodb_mutex.unlock();
}

// -------------------------------------------------------------------------------------------------
// PRIVATE METHODS
// -------------------------------------------------------------------------------------------------

void RedisMongoDB::_setup_mongodb() {
    // TODO Implement
    throw std::runtime_error("Not implemented");
}

void RedisMongoDB::_setup_redis() {
    if (this->redis_cluster) {
        // TODO Implement
        throw std::runtime_error("Not implemented");
    } else {
        this->redis_single = redisConnect(this->redis_host.c_str(), this->redis_port);
        this->redis_cluster = nullptr;
    }
    if (this->redis_single->err) {
        runtime_error("Redis error: " + string(this->redis_single->errstr));
    } else {
        cout << "Connected to Redis at " << this->redis_host << ":" << this->redis_port << endl;
    }

    void RedisMongoDB::_setup_indexes() {
        string url = "mongodb://" + this->mongo_user + ":" + this->mongo_password + "@" +
                     this->mongodb_host + ":" + this->mongo_port;
        try {
            mongocxx::instance instance;
            auto uri = mongocxx::uri{url};
            this->mongodb_pool = new mongocxx::pool(uri);
            auto database = this->mongodb_pool->acquire();
            this->mongodb = database[this->database_name];
            const auto ping_cmd =
                bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("ping", 1));
            this->mongodb.run_command(ping_cmd.view());
            this->mongodb_collection = this->mongodb[this->collection_name];
            std::cout << "Connected to MongoDB at " << this->mongodb_host << ":" << this->mongo_port
                      << endl;
        } catch (const std::exception& e) {
            runtime_error(e.what());
        }
    }

    bsoncxx::document::value RedisMongoDB::_build_node_document(const Node& node) {
        auto document = bsoncxx::builder::basic::document{};
        document.append(bsoncxx::builder::basic::kvp("_id", node._id));
        document.append(bsoncxx::builder::basic::kvp("handle", node.handle));
        document.append(bsoncxx::builder::basic::kvp("type", node.named_type));
        document.append(bsoncxx::builder::basic::kvp("name", node.name));
        return document;
    }

    bsoncxx::document RedisMongoDB::_build_link_document(const Link& link) {
        //    ListOfAny composite_type;

        // string named_type_hash;
        // vector<string> targets;
        // bool is_toplevel = true;
        // TargetsDocuments targets_documents;
        auto document = bsoncxx::builder::basic::document{};
        document.append(bsoncxx::builder::basic::kvp("_id", link._id));
        document.append(bsoncxx::builder::basic::kvp("handle", link.handle));
        document.append(bsoncxx::builder::basic::kvp("type", link.named_type));
        // bson list
        auto targets = bsoncxx::builder::basic::array{};
        for (const auto& item : link.target_documents) {
            if (auto node = std::get_if<Node>(&item)) {
                auto node_document = this->_build_node_document(*node);
                targets.append(node_document);
            } else if (auto link = std::get_if<Link>(&item)) {
                auto link_document = this->_build_link_document(*link);
                targets.append(link_document);
            }
        }
        document.append(bsoncxx::builder::basic::kvp("targets", targets));

        return document;
    }

    void RedisMongoDB::_update_atom_indexes(const optional<const vector<Atom>>& buffer) {
        if (buffer != nullopt) {
            // TODO Implement
            throw std::runtime_error("Not implemented");
        }
        for(auto& document : this->documents_buffer) {


            
            // auto handle = document.view()["_id"].get_utf8().value.to_string();
            // auto type = document.view()["type"].get_utf8().value.to_string();
            // auto targets = document.view()["targets"].get_array().value;
            // for (auto target : targets) {
            //     auto target_handle = target.get_document().view()["_id"].get_utf8().value.to_string();
            //     auto target_type = target.get_document().view()["type"].get_utf8().value.to_string();
            //     auto target_collection = this->mongodb[target_type];
            //     auto filter = bsoncxx::builder::basic::document{};
            //     filter.append(bsoncxx::builder::basic::kvp("_id", target_handle));
            //     auto update = bsoncxx::builder::basic::document{};
            //     update.append(bsoncxx::builder::basic::kvp("$addToSet", bsoncxx::builder::basic::kvp("incoming", handle)));
            //     target_collection.update_one(filter.view(), update.view());
            // }
        }
    }