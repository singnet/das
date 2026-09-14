#include "ProtectedAtomDB.h"

#include "InMemoryDBAPITypes.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"
#include "Utils.h"
#include "Node.h"
#include "Link.h"
#include "Atom.h"

using namespace std;
using namespace atomdb;
using namespace commons;

// --------------------------------------------------------------------------------
// Constructors and destructors

ProtectedAtomDB::ProtectedAtomDB(shared_ptr<AtomDB> backend) : backend(backend) {
    if (this->backend == nullptr) {
        RAISE_ERROR("ProtectedAtomDB requires a non-null backend AtomDB");
    }
    this->uid_ = this->backend->get_uid();
    this->manifest = make_shared<AuthorizationManifest>(backend);
    LOG_INFO("ProtectedAtomDB initialized");
}

// --------------------------------------------------------------------------------
// Public methods

shared_ptr<Atom> ProtectedAtomDB::get_atom(const string& handle, const Keychain& keychain) {
    auto public_key = keychain.get_public_key(this->uid_);

    if (!this->can_read(public_key, handle)) {
        return nullptr;
    }
    return this->backend->get_atom(handle);
}

shared_ptr<Node> ProtectedAtomDB::get_node(const string& handle, const Keychain& keychain) {
    return dynamic_pointer_cast<Node>(this->get_atom(handle, keychain));
}

shared_ptr<Link> ProtectedAtomDB::get_link(const string& handle, const Keychain& keychain) {
    return dynamic_pointer_cast<Link>(this->get_atom(handle, keychain));
}

vector<shared_ptr<Atom>> ProtectedAtomDB::get_matching_atoms(bool is_toplevel,
                                                             Atom& key,
                                                             const Keychain& keychain) {
    RAISE_ERROR("ProtectedAtomDB::get_matching_atoms(..., public_key) is not implemented yet");
}

shared_ptr<atomdb_api_types::HandleSet> ProtectedAtomDB::query_for_pattern(const LinkSchema& link_schema,
                                                                           const Keychain& keychain) {
    RAISE_ERROR("ProtectedAtomDB::query_for_pattern(link_schema, public_key) is not implemented yet");
}

shared_ptr<atomdb_api_types::HandleList> ProtectedAtomDB::query_for_targets(const string& handle,
                                                                            const Keychain& keychain) {
    RAISE_ERROR("ProtectedAtomDB::query_for_targets(handle, public_key) is not implemented yet");
}

shared_ptr<atomdb_api_types::HandleSet> ProtectedAtomDB::query_for_incoming_set(
    const string& handle, const Keychain& keychain) {
    RAISE_ERROR("ProtectedAtomDB::query_for_incoming_set(handle, public_key) is not implemented yet");
}

bool ProtectedAtomDB::atom_exists(const string& handle, const Keychain& keychain) {
    RAISE_ERROR("ProtectedAtomDB::atom_exists(handle, public_key) is not implemented yet");
}

bool ProtectedAtomDB::node_exists(const string& handle, const Keychain& keychain) {
    RAISE_ERROR("ProtectedAtomDB::node_exists(handle, public_key) is not implemented yet");
}

bool ProtectedAtomDB::link_exists(const string& handle, const Keychain& keychain) {
    RAISE_ERROR("ProtectedAtomDB::link_exists(handle, public_key) is not implemented yet");
}

set<string> ProtectedAtomDB::atoms_exist(const vector<string>& handles, const Keychain& keychain) {
    RAISE_ERROR("ProtectedAtomDB::atoms_exist(handles, public_key) is not implemented yet");
}

set<string> ProtectedAtomDB::nodes_exist(const vector<string>& handles, const Keychain& keychain) {
    RAISE_ERROR("ProtectedAtomDB::nodes_exist(handles, public_key) is not implemented yet");
}

set<string> ProtectedAtomDB::links_exist(const vector<string>& handles, const Keychain& keychain) {
    RAISE_ERROR("ProtectedAtomDB::links_exist(handles, public_key) is not implemented yet");
}

string ProtectedAtomDB::add_atom(const atoms::Atom* atom,
                                 const Keychain& keychain,
                                 const atoms::Merger* merger) {
    RAISE_ERROR("ProtectedAtomDB::add_atom(atom, public_key) is not implemented yet");
}

string ProtectedAtomDB::add_node(const atoms::Node* node,
                                 const Keychain& keychain,
                                 const atoms::Merger* merger) {
    RAISE_ERROR("ProtectedAtomDB::add_node(node, public_key) is not implemented yet");
}

string ProtectedAtomDB::add_link(const atoms::Link* link,
                                 const Keychain& keychain,
                                 const atoms::Merger* merger) {
    RAISE_ERROR("ProtectedAtomDB::add_link(link, public_key) is not implemented yet");
}

vector<string> ProtectedAtomDB::add_atoms(const vector<atoms::Atom*>& atom_list,
                                          const Keychain& keychain,
                                          bool is_transactional,
                                          const atoms::Merger* merger) {
    RAISE_ERROR("ProtectedAtomDB::add_atoms(atom_list, public_key) is not implemented yet");
}

vector<string> ProtectedAtomDB::add_nodes(const vector<atoms::Node*>& nodes,
                                          const Keychain& keychain,
                                          bool is_transactional,
                                          const atoms::Merger* merger) {
    RAISE_ERROR("ProtectedAtomDB::add_nodes(nodes, public_key) is not implemented yet");
}

vector<string> ProtectedAtomDB::add_links(const vector<atoms::Link*>& links,
                                          const Keychain& keychain,
                                          bool is_transactional,
                                          const atoms::Merger* merger) {
    RAISE_ERROR("ProtectedAtomDB::add_links(links, public_key) is not implemented yet");
}

bool ProtectedAtomDB::delete_atom(const string& handle,
                                  const Keychain& keychain,
                                  bool delete_link_targets) {
    RAISE_ERROR("ProtectedAtomDB::delete_atom(handle, public_key) is not implemented yet");
}

bool ProtectedAtomDB::delete_node(const string& handle,
                                  const Keychain& keychain,
                                  bool delete_link_targets) {
    RAISE_ERROR("ProtectedAtomDB::delete_node(handle, public_key) is not implemented yet");
}

bool ProtectedAtomDB::delete_link(const string& handle,
                                  const Keychain& keychain,
                                  bool delete_link_targets) {
    RAISE_ERROR("ProtectedAtomDB::delete_link(handle, public_key) is not implemented yet");
}

uint ProtectedAtomDB::delete_atoms(const vector<string>& handles,
                                   const Keychain& keychain,
                                   bool delete_link_targets) {
    RAISE_ERROR("ProtectedAtomDB::delete_atoms(handles, public_key) is not implemented yet");
}

uint ProtectedAtomDB::delete_nodes(const vector<string>& handles,
                                   const Keychain& keychain,
                                   bool delete_link_targets) {
    RAISE_ERROR("ProtectedAtomDB::delete_nodes(handles, public_key) is not implemented yet");
}

uint ProtectedAtomDB::delete_links(const vector<string>& handles,
                                   const Keychain& keychain,
                                   bool delete_link_targets) {
    RAISE_ERROR("ProtectedAtomDB::delete_links(handles, public_key) is not implemented yet");
}

void ProtectedAtomDB::re_index_patterns(const Keychain& keychain, bool flush_patterns) {
    RAISE_ERROR("ProtectedAtomDB::re_index_patterns(public_key) is not implemented yet");
}

size_t ProtectedAtomDB::node_count(const Keychain& keychain) const {
    RAISE_ERROR("ProtectedAtomDB::node_count(public_key) is not implemented yet");
}

size_t ProtectedAtomDB::link_count(const Keychain& keychain) const {
    RAISE_ERROR("ProtectedAtomDB::link_count(public_key) is not implemented yet");
}

size_t ProtectedAtomDB::atom_count(const Keychain& keychain) const {
    RAISE_ERROR("ProtectedAtomDB::atom_count(public_key) is not implemented yet");
}

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

bool ProtectedAtomDB::ensure_registered(const string& public_key) {
    if (this->manifest->is_registered(public_key)) {
        return true;
    }

    auto access_document = this->backend->get_access_permission(public_key);

    if (access_document->get_access_key() != public_key) {
        return false;
    }

    this->manifest->add_document(access_document);

    return this->manifest->is_registered(public_key);
}

bool ProtectedAtomDB::can_read(const string& public_key, const string& handle) {
    if (!this->ensure_registered(public_key)) {
        return false;
    }
    return this->manifest->is_granted(public_key, handle, AuthorizationOperation::READ);
}

bool ProtectedAtomDB::can_read(const Keychain& keychain, const atoms::Atom& atom) {
    if (!this->ensure_registered(public_key)) {
        return false;
    }

    HandleDecoder& decoder = *this->backend;
    return this->manifest->is_authorized(
        atom, public_key.keys[0], AuthorizationOperation::READ, decoder);
}

bool ProtectedAtomDB::can_write(const Keychain& keychain, const atoms::Atom& atom) {
    if (!this->ensure_registered(public_key)) {
        return false;
    }

    HandleDecoder& decoder = *this->backend;
    return this->manifest->is_authorized(
        atom, public_key.keys[0], AuthorizationOperation::WRITE, decoder);
}

bool ProtectedAtomDB::can_write(const Keychain& keychain, const string& handle) {
    if (!this->ensure_registered(public_key)) {
        return false;
    }

    HandleDecoder& decoder = *this->backend;
    return this->manifest->is_authorized(
        handle, public_key.keys[0], AuthorizationOperation::WRITE, decoder);
}

shared_ptr<atomdb_api_types::HandleSet> ProtectedAtomDB::filter_handle_set(
    shared_ptr<atomdb_api_types::HandleSet> raw, const Keychain& keychain) {
    auto filtered = make_shared<atomdb_api_types::HandleSetInMemory>();
    if (raw == nullptr) {
        return filtered;
    }
    bool copy_metadata = this->backend->allow_nested_indexing();
    auto it = raw->get_iterator();
    while (true) {
        char* handle_cstr = it->next();
        if (handle_cstr == nullptr) {
            break;
        }
        string handle(handle_cstr);
        if (!this->can_read(public_key, handle)) {
            continue;
        }
        if (copy_metadata) {
            filtered->add_handle(handle,
                                 raw->get_metta_expressions_by_handle(handle),
                                 raw->get_assignments_by_handle(handle));
        } else {
            filtered->add_handle(handle);
        }
    }
    return filtered;
}

shared_ptr<atomdb_api_types::HandleList> ProtectedAtomDB::filter_handle_list(
    shared_ptr<atomdb_api_types::HandleList> raw, const Keychain& keychain) {
    auto filtered = make_shared<atomdb_api_types::HandleListInMemory>();

    if (raw == nullptr) {
        return filtered;
    }

    for (unsigned int i = 0; i < raw->size(); ++i) {
        const char* handle_cstr = raw->get_handle(i);
        if (handle_cstr == nullptr) {
            continue;
        }
        string handle(handle_cstr);
        if (this->can_read(public_key, handle)) {
            filtered->add_handle(handle);
        }
    }
    return filtered;
}