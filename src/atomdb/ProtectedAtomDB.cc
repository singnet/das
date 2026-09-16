#include "ProtectedAtomDB.h"

#include "InMemoryDBAPITypes.h"

#define LOG_LEVEL INFO_LEVEL
#include "Atom.h"
#include "Link.h"
#include "Logger.h"
#include "Node.h"
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
    this->uid_ = this->backend->get_uid();
    this->manifest = make_shared<AuthorizationManifest>(backend);
    LOG_INFO("ProtectedAtomDB initialized");
}

// --------------------------------------------------------------------------------
// Public methods

shared_ptr<Atom> ProtectedAtomDB::get_atom(const string& handle, shared_ptr<Keychain> keychain) {
    if (!this->can_read(keychain, handle)) {
        return nullptr;
    }
    return this->backend->get_atom(handle);
}

shared_ptr<Node> ProtectedAtomDB::get_node(const string& handle, shared_ptr<Keychain> keychain) {
    if (!this->can_read(keychain, handle)) {
        return nullptr;
    }
    return this->backend->get_node(handle);
}

shared_ptr<Link> ProtectedAtomDB::get_link(const string& handle, shared_ptr<Keychain> keychain) {
    if (!this->can_read(keychain, handle)) {
        return nullptr;
    }
    return this->backend->get_link(handle);
}

vector<shared_ptr<Atom>> ProtectedAtomDB::get_matching_atoms(bool is_toplevel,
                                                             Atom& key,
                                                             shared_ptr<Keychain> keychain) {
    vector<shared_ptr<Atom>> original_matching_atoms = this->backend->get_matching_atoms(is_toplevel, key);

    if (original_matching_atoms.empty()) {
        return {};
    }

    vector<shared_ptr<Atom>> authorized_matching_atoms;

    for (const auto& atom : original_matching_atoms) {
        if (this->can_read(keychain, atom->handle())) {
            authorized_matching_atoms.push_back(atom);
        };
    }

    return authorized_matching_atoms;
}

shared_ptr<atomdb_api_types::HandleSet> ProtectedAtomDB::query_for_pattern(
    const LinkSchema& link_schema, shared_ptr<Keychain> keychain) {
    return this->filter_handle_set(this->backend->query_for_pattern(link_schema), keychain);
}

shared_ptr<atomdb_api_types::HandleList> ProtectedAtomDB::query_for_targets(
    const string& handle, shared_ptr<Keychain> keychain) {
    return this->filter_handle_list(this->backend->query_for_targets(handle), keychain);
}

shared_ptr<atomdb_api_types::HandleSet> ProtectedAtomDB::query_for_incoming_set(
    const string& handle, shared_ptr<Keychain> keychain) {
    return this->filter_handle_set(this->backend->query_for_incoming_set(handle), keychain);
}

bool ProtectedAtomDB::atom_exists(const string& handle, shared_ptr<Keychain> keychain) {
    if (!this->can_read(keychain, handle)) {
        return false;
    }
    return this->backend->atom_exists(handle);
}

bool ProtectedAtomDB::node_exists(const string& handle, shared_ptr<Keychain> keychain) {
    if (!this->can_read(keychain, handle)) {
        return false;
    }
    return this->backend->node_exists(handle);
}

bool ProtectedAtomDB::link_exists(const string& handle, shared_ptr<Keychain> keychain) {
    if (!this->can_read(keychain, handle)) {
        return false;
    }
    return this->backend->link_exists(handle);
}

set<string> ProtectedAtomDB::atoms_exist(const vector<string>& handles, shared_ptr<Keychain> keychain) {
    return this->filter_handles(this->backend->atoms_exist(handles), keychain);
}

set<string> ProtectedAtomDB::nodes_exist(const vector<string>& handles, shared_ptr<Keychain> keychain) {
    return this->filter_handles(this->backend->nodes_exist(handles), keychain);
}

set<string> ProtectedAtomDB::links_exist(const vector<string>& handles, shared_ptr<Keychain> keychain) {
    return this->filter_handles(this->backend->links_exist(handles), keychain);
}

string ProtectedAtomDB::add_atom(const atoms::Atom* atom,
                                 shared_ptr<Keychain> keychain,
                                 const atoms::Merger* merger) {
    RAISE_ERROR("ProtectedAtomDB::add_atom(atom, public_key) is not implemented yet");
}

string ProtectedAtomDB::add_node(const atoms::Node* node,
                                 shared_ptr<Keychain> keychain,
                                 const atoms::Merger* merger) {
    RAISE_ERROR("ProtectedAtomDB::add_node(node, public_key) is not implemented yet");
}

string ProtectedAtomDB::add_link(const atoms::Link* link,
                                 shared_ptr<Keychain> keychain,
                                 const atoms::Merger* merger) {
    RAISE_ERROR("ProtectedAtomDB::add_link(link, public_key) is not implemented yet");
}

vector<string> ProtectedAtomDB::add_atoms(const vector<atoms::Atom*>& atom_list,
                                          shared_ptr<Keychain> keychain,
                                          bool is_transactional,
                                          const atoms::Merger* merger) {
    RAISE_ERROR("ProtectedAtomDB::add_atoms(atom_list, public_key) is not implemented yet");
}

vector<string> ProtectedAtomDB::add_nodes(const vector<atoms::Node*>& nodes,
                                          shared_ptr<Keychain> keychain,
                                          bool is_transactional,
                                          const atoms::Merger* merger) {
    RAISE_ERROR("ProtectedAtomDB::add_nodes(nodes, public_key) is not implemented yet");
}

vector<string> ProtectedAtomDB::add_links(const vector<atoms::Link*>& links,
                                          shared_ptr<Keychain> keychain,
                                          bool is_transactional,
                                          const atoms::Merger* merger) {
    RAISE_ERROR("ProtectedAtomDB::add_links(links, public_key) is not implemented yet");
}

bool ProtectedAtomDB::delete_atom(const string& handle,
                                  shared_ptr<Keychain> keychain,
                                  bool delete_link_targets) {
    RAISE_ERROR("ProtectedAtomDB::delete_atom(handle, public_key) is not implemented yet");
}

bool ProtectedAtomDB::delete_node(const string& handle,
                                  shared_ptr<Keychain> keychain,
                                  bool delete_link_targets) {
    RAISE_ERROR("ProtectedAtomDB::delete_node(handle, public_key) is not implemented yet");
}

bool ProtectedAtomDB::delete_link(const string& handle,
                                  shared_ptr<Keychain> keychain,
                                  bool delete_link_targets) {
    RAISE_ERROR("ProtectedAtomDB::delete_link(handle, public_key) is not implemented yet");
}

uint ProtectedAtomDB::delete_atoms(const vector<string>& handles,
                                   shared_ptr<Keychain> keychain,
                                   bool delete_link_targets) {
    RAISE_ERROR("ProtectedAtomDB::delete_atoms(handles, public_key) is not implemented yet");
}

uint ProtectedAtomDB::delete_nodes(const vector<string>& handles,
                                   shared_ptr<Keychain> keychain,
                                   bool delete_link_targets) {
    RAISE_ERROR("ProtectedAtomDB::delete_nodes(handles, public_key) is not implemented yet");
}

uint ProtectedAtomDB::delete_links(const vector<string>& handles,
                                   shared_ptr<Keychain> keychain,
                                   bool delete_link_targets) {
    RAISE_ERROR("ProtectedAtomDB::delete_links(handles, public_key) is not implemented yet");
}

void ProtectedAtomDB::re_index_patterns(shared_ptr<Keychain> keychain, bool flush_patterns) {
    RAISE_ERROR("ProtectedAtomDB::re_index_patterns(public_key) is not implemented yet");
}

size_t ProtectedAtomDB::node_count(shared_ptr<Keychain> keychain) const {
    RAISE_ERROR("ProtectedAtomDB::node_count(public_key) is not implemented yet");
}

size_t ProtectedAtomDB::link_count(shared_ptr<Keychain> keychain) const {
    RAISE_ERROR("ProtectedAtomDB::link_count(public_key) is not implemented yet");
}

size_t ProtectedAtomDB::atom_count(shared_ptr<Keychain> keychain) const {
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
                "a Keychain.");
}

bool ProtectedAtomDB::can_read(shared_ptr<Keychain> keychain, const string& handle) {
    auto public_key = keychain->get_public_key(this->uid_);
    if (!this->ensure_registered(public_key)) {
        return false;
    }
    return this->manifest->is_granted(public_key, handle, AuthorizationOperation::READ);
}

bool ProtectedAtomDB::can_write(shared_ptr<Keychain> keychain, const string& handle) {
    auto public_key = keychain->get_public_key(this->uid_);
    if (!this->ensure_registered(public_key)) {
        return false;
    }
    return this->manifest->is_granted(public_key, handle, AuthorizationOperation::WRITE);
}

bool ProtectedAtomDB::can_read(shared_ptr<Keychain> keychain, const atoms::Atom& atom) {
    RAISE_ERROR("ProtectedAtomDB::can_read is not implemented yet.");
}

bool ProtectedAtomDB::can_write(shared_ptr<Keychain> keychain, const atoms::Atom& atom) {
    RAISE_ERROR("ProtectedAtomDB::can_write is not implemented yet.");
}

bool ProtectedAtomDB::ensure_registered(const string& public_key) {
    if (this->manifest->is_registered(public_key)) {
        return true;
    }

    auto access_document = this->backend->get_access_permissions(public_key);

    if (access_document == nullptr) {
        return false;
    }

    if (access_document->get_access_key() != public_key) {
        return false;
    }

    this->manifest->add_document(access_document);

    return this->manifest->is_registered(public_key);
}

shared_ptr<atomdb_api_types::HandleSet> ProtectedAtomDB::filter_handle_set(
    shared_ptr<atomdb_api_types::HandleSet> original_handle_set, shared_ptr<Keychain> keychain) {
    auto authorized_handle_set = make_shared<atomdb_api_types::HandleSetInMemory>();

    if (original_handle_set == nullptr) {
        return authorized_handle_set;
    }

    auto it = original_handle_set->get_iterator();

    while (true) {
        char* handle_cstr = it->next();
        if (!handle_cstr) break;
        string handle(handle_cstr);

        if (!this->can_read(keychain, handle)) continue;

        authorized_handle_set->add_handle(handle,
                                          original_handle_set->get_metta_expressions_by_handle(handle),
                                          original_handle_set->get_assignments_by_handle(handle));
    }

#if LOG_LEVEL >= DEBUG_LEVEL
    LOG_DEBUG("[ ProtectedAtomDB::filter_handle_set() - original handle_set: ]" +
              std::to_string(original_handle_set->size()));
    LOG_DEBUG("[ ProtectedAtomDB::filter_handle_set() - authorized handle_set: ]" +
              std::to_string(authorized_handle_set->size()));
#endif

    return authorized_handle_set;
}

shared_ptr<atomdb_api_types::HandleList> ProtectedAtomDB::filter_handle_list(
    shared_ptr<atomdb_api_types::HandleList> original_handle_list, shared_ptr<Keychain> keychain) {
    auto authorized_handle_list = make_shared<atomdb_api_types::HandleListInMemory>();

    if (original_handle_list == nullptr) {
        return authorized_handle_list;
    }

    for (unsigned int i = 0; i < original_handle_list->size(); ++i) {
        const char* handle_cstr = original_handle_list->get_handle(i);
        if (handle_cstr == nullptr) {
            continue;
        }
        string handle(handle_cstr);
        if (this->can_read(keychain, handle)) {
            authorized_handle_list->add_handle(handle);
        }
    }

#if LOG_LEVEL >= DEBUG_LEVEL
    LOG_DEBUG("[ ProtectedAtomDB::filter_handle_list() - original handle_list: ]" +
              std::to_string(original_handle_list->size()));
    LOG_DEBUG("[ ProtectedAtomDB::filter_handle_list() - authorized handle_list: ]" +
              std::to_string(authorized_handle_list->size()));
#endif

    return authorized_handle_list;
}

set<string> ProtectedAtomDB::filter_handles(set<string> original_handles,
                                            shared_ptr<Keychain> keychain) {
    set<string> authorized_handles;

    for (const auto& handle : original_handles) {
        if (!this->can_read(keychain, handle)) continue;
        authorized_handles.insert(handle);
    }

    return authorized_handles;
}