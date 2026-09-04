#include "ProtectedAtomDB.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"
#include "Utils.h"

using namespace std;
using namespace atomdb;
using namespace commons;

// --------------------------------------------------------------------------------
// Constructors and destructors

ProtectedAtomDB::ProtectedAtomDB(shared_ptr<AtomDB> backend) : backend(backend) {
    if (this->backend == nullptr) {
        RAISE_ERROR("ProtectedAtomDB requires a non-null backend AtomDB");
    }
    this->manifest = make_shared<AuthorizationManifest>(backend);
    LOG_INFO("ProtectedAtomDB initialized");
}

// --------------------------------------------------------------------------------
// Public methods

shared_ptr<Atom> ProtectedAtomDB::get_atom(const string& handle,
                                           const atomdb_api_types::PublicKey& public_key) {
    RAISE_ERROR("ProtectedAtomDB::get_atom(handle, public_key) is not implemented yet");
}

shared_ptr<Node> ProtectedAtomDB::get_node(const string& handle,
                                           const atomdb_api_types::PublicKey& public_key) {
    RAISE_ERROR("ProtectedAtomDB::get_node(handle, public_key) is not implemented yet");
}

shared_ptr<Link> ProtectedAtomDB::get_link(const string& handle,
                                           const atomdb_api_types::PublicKey& public_key) {
    RAISE_ERROR("ProtectedAtomDB::get_link(handle, public_key) is not implemented yet");
}

vector<shared_ptr<Atom>> ProtectedAtomDB::get_matching_atoms(
    bool is_toplevel, Atom& key, const atomdb_api_types::PublicKey& public_key) {
    RAISE_ERROR("ProtectedAtomDB::get_matching_atoms(..., public_key) is not implemented yet");
}

shared_ptr<atomdb_api_types::HandleSet> ProtectedAtomDB::query_for_pattern(
    const LinkSchema& link_schema, const atomdb_api_types::PublicKey& public_key) {
    RAISE_ERROR("ProtectedAtomDB::query_for_pattern(link_schema, public_key) is not implemented yet");
}

shared_ptr<atomdb_api_types::HandleList> ProtectedAtomDB::query_for_targets(
    const string& handle, const atomdb_api_types::PublicKey& public_key) {
    RAISE_ERROR("ProtectedAtomDB::query_for_targets(handle, public_key) is not implemented yet");
}

shared_ptr<atomdb_api_types::HandleSet> ProtectedAtomDB::query_for_incoming_set(
    const string& handle, const atomdb_api_types::PublicKey& public_key) {
    RAISE_ERROR("ProtectedAtomDB::query_for_incoming_set(handle, public_key) is not implemented yet");
}

bool ProtectedAtomDB::atom_exists(const string& handle, const atomdb_api_types::PublicKey& public_key) {
    RAISE_ERROR("ProtectedAtomDB::atom_exists(handle, public_key) is not implemented yet");
}

bool ProtectedAtomDB::node_exists(const string& handle, const atomdb_api_types::PublicKey& public_key) {
    RAISE_ERROR("ProtectedAtomDB::node_exists(handle, public_key) is not implemented yet");
}

bool ProtectedAtomDB::link_exists(const string& handle, const atomdb_api_types::PublicKey& public_key) {
    RAISE_ERROR("ProtectedAtomDB::link_exists(handle, public_key) is not implemented yet");
}

set<string> ProtectedAtomDB::atoms_exist(const vector<string>& handles,
                                         const atomdb_api_types::PublicKey& public_key) {
    RAISE_ERROR("ProtectedAtomDB::atoms_exist(handles, public_key) is not implemented yet");
}

set<string> ProtectedAtomDB::nodes_exist(const vector<string>& handles,
                                         const atomdb_api_types::PublicKey& public_key) {
    RAISE_ERROR("ProtectedAtomDB::nodes_exist(handles, public_key) is not implemented yet");
}

set<string> ProtectedAtomDB::links_exist(const vector<string>& handles,
                                         const atomdb_api_types::PublicKey& public_key) {
    RAISE_ERROR("ProtectedAtomDB::links_exist(handles, public_key) is not implemented yet");
}

string ProtectedAtomDB::add_atom(const atoms::Atom* atom,
                                 const atomdb_api_types::PublicKey& public_key,
                                 const atoms::Merger* merger) {
    RAISE_ERROR("ProtectedAtomDB::add_atom(atom, public_key) is not implemented yet");
}

string ProtectedAtomDB::add_node(const atoms::Node* node,
                                 const atomdb_api_types::PublicKey& public_key,
                                 const atoms::Merger* merger) {
    RAISE_ERROR("ProtectedAtomDB::add_node(node, public_key) is not implemented yet");
}

string ProtectedAtomDB::add_link(const atoms::Link* link,
                                 const atomdb_api_types::PublicKey& public_key,
                                 const atoms::Merger* merger) {
    RAISE_ERROR("ProtectedAtomDB::add_link(link, public_key) is not implemented yet");
}

vector<string> ProtectedAtomDB::add_atoms(const vector<atoms::Atom*>& atom_list,
                                          const atomdb_api_types::PublicKey& public_key,
                                          bool is_transactional,
                                          const atoms::Merger* merger) {
    RAISE_ERROR("ProtectedAtomDB::add_atoms(atom_list, public_key) is not implemented yet");
}

vector<string> ProtectedAtomDB::add_nodes(const vector<atoms::Node*>& nodes,
                                          const atomdb_api_types::PublicKey& public_key,
                                          bool is_transactional,
                                          const atoms::Merger* merger) {
    RAISE_ERROR("ProtectedAtomDB::add_nodes(nodes, public_key) is not implemented yet");
}

vector<string> ProtectedAtomDB::add_links(const vector<atoms::Link*>& links,
                                          const atomdb_api_types::PublicKey& public_key,
                                          bool is_transactional,
                                          const atoms::Merger* merger) {
    RAISE_ERROR("ProtectedAtomDB::add_links(links, public_key) is not implemented yet");
}

bool ProtectedAtomDB::delete_atom(const string& handle,
                                  const atomdb_api_types::PublicKey& public_key,
                                  bool delete_link_targets) {
    RAISE_ERROR("ProtectedAtomDB::delete_atom(handle, public_key) is not implemented yet");
}

bool ProtectedAtomDB::delete_node(const string& handle,
                                  const atomdb_api_types::PublicKey& public_key,
                                  bool delete_link_targets) {
    RAISE_ERROR("ProtectedAtomDB::delete_node(handle, public_key) is not implemented yet");
}

bool ProtectedAtomDB::delete_link(const string& handle,
                                  const atomdb_api_types::PublicKey& public_key,
                                  bool delete_link_targets) {
    RAISE_ERROR("ProtectedAtomDB::delete_link(handle, public_key) is not implemented yet");
}

uint ProtectedAtomDB::delete_atoms(const vector<string>& handles,
                                   const atomdb_api_types::PublicKey& public_key,
                                   bool delete_link_targets) {
    RAISE_ERROR("ProtectedAtomDB::delete_atoms(handles, public_key) is not implemented yet");
}

uint ProtectedAtomDB::delete_nodes(const vector<string>& handles,
                                   const atomdb_api_types::PublicKey& public_key,
                                   bool delete_link_targets) {
    RAISE_ERROR("ProtectedAtomDB::delete_nodes(handles, public_key) is not implemented yet");
}

uint ProtectedAtomDB::delete_links(const vector<string>& handles,
                                   const atomdb_api_types::PublicKey& public_key,
                                   bool delete_link_targets) {
    RAISE_ERROR("ProtectedAtomDB::delete_links(handles, public_key) is not implemented yet");
}

void ProtectedAtomDB::re_index_patterns(const atomdb_api_types::PublicKey& public_key,
                                        bool flush_patterns) {
    RAISE_ERROR("ProtectedAtomDB::re_index_patterns(public_key) is not implemented yet");
}

size_t ProtectedAtomDB::node_count(const atomdb_api_types::PublicKey& public_key) const {
    RAISE_ERROR("ProtectedAtomDB::node_count(public_key) is not implemented yet");
}

size_t ProtectedAtomDB::link_count(const atomdb_api_types::PublicKey& public_key) const {
    RAISE_ERROR("ProtectedAtomDB::link_count(public_key) is not implemented yet");
}

size_t ProtectedAtomDB::atom_count(const atomdb_api_types::PublicKey& public_key) const {
    RAISE_ERROR("ProtectedAtomDB::atom_count(public_key) is not implemented yet");
}

bool ProtectedAtomDB::allow_nested_indexing() { return this->backend->allow_nested_indexing(); }

bool ProtectedAtomDB::composite_type_enabled() const { return this->backend->composite_type_enabled(); }

atomdb_api_types::ProtectionMode ProtectedAtomDB::get_protection_mode() const {
    return this->backend->get_protection_mode();
}

// --------------------------------------------------------------------------------
// Public methods (without public_key - reject the call)

shared_ptr<Atom> ProtectedAtomDB::get_atom(const string& handle) {
    raise_public_key_required("get_atom");
}

shared_ptr<Node> ProtectedAtomDB::get_node(const string& handle) {
    raise_public_key_required("get_node");
}

shared_ptr<Link> ProtectedAtomDB::get_link(const string& handle) {
    raise_public_key_required("get_link");
}

vector<shared_ptr<Atom>> ProtectedAtomDB::get_matching_atoms(bool is_toplevel, Atom& key) {
    raise_public_key_required("get_matching_atoms");
}

shared_ptr<atomdb_api_types::HandleSet> ProtectedAtomDB::query_for_pattern(
    const LinkSchema& link_schema) {
    raise_public_key_required("query_for_pattern");
}

shared_ptr<atomdb_api_types::HandleList> ProtectedAtomDB::query_for_targets(const string& handle) {
    raise_public_key_required("query_for_targets");
}

shared_ptr<atomdb_api_types::HandleSet> ProtectedAtomDB::query_for_incoming_set(const string& handle) {
    raise_public_key_required("query_for_incoming_set");
}

bool ProtectedAtomDB::atom_exists(const string& handle) { raise_public_key_required("atom_exists"); }

bool ProtectedAtomDB::node_exists(const string& handle) { raise_public_key_required("node_exists"); }

bool ProtectedAtomDB::link_exists(const string& handle) { raise_public_key_required("link_exists"); }

set<string> ProtectedAtomDB::atoms_exist(const vector<string>& handles) {
    raise_public_key_required("atoms_exist");
}

set<string> ProtectedAtomDB::nodes_exist(const vector<string>& handles) {
    raise_public_key_required("nodes_exist");
}

set<string> ProtectedAtomDB::links_exist(const vector<string>& handles) {
    raise_public_key_required("links_exist");
}

string ProtectedAtomDB::add_atom(const atoms::Atom* atom, const atoms::Merger* merger) {
    raise_public_key_required("add_atom");
}

string ProtectedAtomDB::add_node(const atoms::Node* node, const atoms::Merger* merger) {
    raise_public_key_required("add_node");
}

string ProtectedAtomDB::add_link(const atoms::Link* link, const atoms::Merger* merger) {
    raise_public_key_required("add_link");
}

vector<string> ProtectedAtomDB::add_atoms(const vector<atoms::Atom*>& atom_list,
                                          bool is_transactional,
                                          const atoms::Merger* merger) {
    raise_public_key_required("add_atoms");
}

vector<string> ProtectedAtomDB::add_nodes(const vector<atoms::Node*>& nodes,
                                          bool is_transactional,
                                          const atoms::Merger* merger) {
    raise_public_key_required("add_nodes");
}

vector<string> ProtectedAtomDB::add_links(const vector<atoms::Link*>& links,
                                          bool is_transactional,
                                          const atoms::Merger* merger) {
    raise_public_key_required("add_links");
}

bool ProtectedAtomDB::delete_atom(const string& handle, bool delete_link_targets) {
    raise_public_key_required("delete_atom");
}

bool ProtectedAtomDB::delete_node(const string& handle, bool delete_link_targets) {
    raise_public_key_required("delete_node");
}

bool ProtectedAtomDB::delete_link(const string& handle, bool delete_link_targets) {
    raise_public_key_required("delete_link");
}

uint ProtectedAtomDB::delete_atoms(const vector<string>& handles, bool delete_link_targets) {
    raise_public_key_required("delete_atoms");
}

uint ProtectedAtomDB::delete_nodes(const vector<string>& handles, bool delete_link_targets) {
    raise_public_key_required("delete_nodes");
}

uint ProtectedAtomDB::delete_links(const vector<string>& handles, bool delete_link_targets) {
    raise_public_key_required("delete_links");
}

void ProtectedAtomDB::re_index_patterns(bool flush_patterns) {
    raise_public_key_required("re_index_patterns");
}

size_t ProtectedAtomDB::node_count() const { raise_public_key_required("node_count"); }

size_t ProtectedAtomDB::link_count() const { raise_public_key_required("link_count"); }

size_t ProtectedAtomDB::atom_count() const { raise_public_key_required("atom_count"); }

// --------------------------------------------------------------------------------
// Private methods

void ProtectedAtomDB::raise_public_key_required(const string& method_name) {
    RAISE_ERROR("ProtectedAtomDB::" + method_name +
                "() is unavailable in protected AtomDBs. Use the public API in ProtectedAtomDB passing "
                "a PublicKey.");
}
