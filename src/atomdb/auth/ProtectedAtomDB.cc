#include "ProtectedAtomDB.h"

using namespace atomdb;

ProtectedAtomDB::ProtectedAtomDB(shared_ptr<AtomDB> backend, const JsonConfig& config)
    : backend(std::move(backend)), config(config) {}

bool ProtectedAtomDB::allow_nested_indexing(const string& public_key) { return false; }

bool ProtectedAtomDB::composite_type_enabled() const { return false; }

bool ProtectedAtomDB::is_protected() const { return true; }

shared_ptr<Atom> ProtectedAtomDB::get_atom(const string& handle, const string& public_key) {
    return nullptr;
}

shared_ptr<Node> ProtectedAtomDB::get_node(const string& handle, const string& public_key) {
    return nullptr;
}

shared_ptr<Link> ProtectedAtomDB::get_link(const string& handle, const string& public_key) {
    return nullptr;
}

vector<shared_ptr<Atom>> ProtectedAtomDB::get_matching_atoms(bool is_toplevel,
                                                             Atom& key,
                                                             const string& public_key) {
    return {};
}

shared_ptr<atomdb_api_types::HandleSet> ProtectedAtomDB::query_for_pattern(
    const LinkSchema& link_schema, const string& public_key) {
    return nullptr;
}

shared_ptr<atomdb_api_types::HandleList> ProtectedAtomDB::query_for_targets(const string& handle,
                                                                            const string& public_key) {
    return nullptr;
}

shared_ptr<atomdb_api_types::HandleSet> ProtectedAtomDB::query_for_incoming_set(
    const string& handle, const string& public_key) {
    return nullptr;
}

bool ProtectedAtomDB::atom_exists(const string& handle, const string& public_key) { return false; }

bool ProtectedAtomDB::node_exists(const string& handle, const string& public_key) { return false; }

bool ProtectedAtomDB::link_exists(const string& handle, const string& public_key) { return false; }

set<string> ProtectedAtomDB::atoms_exist(const vector<string>& handles, const string& public_key) {
    return {};
}

set<string> ProtectedAtomDB::nodes_exist(const vector<string>& handles, const string& public_key) {
    return {};
}

set<string> ProtectedAtomDB::links_exist(const vector<string>& handles, const string& public_key) {
    return {};
}

string ProtectedAtomDB::add_atom(const atoms::Atom* atom,
                                 const string& public_key,
                                 bool throw_if_exists) {
    return "";
}

string ProtectedAtomDB::add_node(const atoms::Node* node,
                                 const string& public_key,
                                 bool throw_if_exists) {
    return "";
}

string ProtectedAtomDB::add_link(const atoms::Link* link,
                                 const string& public_key,
                                 bool throw_if_exists) {
    return "";
}

vector<string> ProtectedAtomDB::add_atoms(const vector<atoms::Atom*>& atoms,
                                          const string& public_key,
                                          bool throw_if_exists,
                                          bool is_transactional) {
    return {};
}

vector<string> ProtectedAtomDB::add_nodes(const vector<atoms::Node*>& nodes,
                                          const string& public_key,
                                          bool throw_if_exists,
                                          bool is_transactional) {
    return {};
}

vector<string> ProtectedAtomDB::add_links(const vector<atoms::Link*>& links,
                                          const string& public_key,
                                          bool throw_if_exists,
                                          bool is_transactional) {
    return {};
}

bool ProtectedAtomDB::delete_atom(const string& handle,
                                  const string& public_key,
                                  bool delete_link_targets) {
    return false;
}

bool ProtectedAtomDB::delete_node(const string& handle,
                                  const string& public_key,
                                  bool delete_link_targets) {
    return false;
}

bool ProtectedAtomDB::delete_link(const string& handle,
                                  const string& public_key,
                                  bool delete_link_targets) {
    return false;
}

uint ProtectedAtomDB::delete_atoms(const vector<string>& handles,
                                   const string& public_key,
                                   bool delete_link_targets) {
    return 0;
}

uint ProtectedAtomDB::delete_nodes(const vector<string>& handles,
                                   const string& public_key,
                                   bool delete_link_targets) {
    return 0;
}

uint ProtectedAtomDB::delete_links(const vector<string>& handles,
                                   const string& public_key,
                                   bool delete_link_targets) {
    return 0;
}

void ProtectedAtomDB::re_index_patterns(const string& public_key, bool flush_patterns) {}

size_t ProtectedAtomDB::node_count(const string& public_key) const { return 0; }

size_t ProtectedAtomDB::link_count(const string& public_key) const { return 0; }

size_t ProtectedAtomDB::atom_count(const string& public_key) const { return 0; }

bool ProtectedAtomDB::can_read(const string& public_key, const string& handle) { return false; }

bool ProtectedAtomDB::can_read(const string& public_key, const atoms::Atom& atom) { return false; }

shared_ptr<atomdb_api_types::HandleSet> ProtectedAtomDB::filter_handle_set(
    shared_ptr<atomdb_api_types::HandleSet> raw, const string& public_key) {
    return nullptr;
}

shared_ptr<atomdb_api_types::HandleList> ProtectedAtomDB::filter_handle_list(
    shared_ptr<atomdb_api_types::HandleList> raw, const string& public_key) {
    return nullptr;
}
