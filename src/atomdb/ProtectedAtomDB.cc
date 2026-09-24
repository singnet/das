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
    auto atom = this->backend->get_atom(handle);
    if (this->authorize_read(atom, keychain)) {
        return atom;
    }
    return nullptr;
}

shared_ptr<Node> ProtectedAtomDB::get_node(const string& handle, shared_ptr<Keychain> keychain) {
    return dynamic_pointer_cast<Node>(this->get_atom(handle, keychain));
}

shared_ptr<Link> ProtectedAtomDB::get_link(const string& handle, shared_ptr<Keychain> keychain) {
    return dynamic_pointer_cast<Link>(this->get_atom(handle, keychain));
}

vector<shared_ptr<Atom>> ProtectedAtomDB::get_matching_atoms(bool is_toplevel,
                                                             Atom& key,
                                                             shared_ptr<Keychain> keychain) {
    auto public_key = this->try_get_public_key(keychain);
    if (!public_key) {
        return {};
    }
    return this->filter_atoms(this->backend->get_matching_atoms(is_toplevel, key), *public_key);
}

shared_ptr<atomdb_api_types::HandleSet> ProtectedAtomDB::query_for_pattern(
    const LinkSchema& link_schema, shared_ptr<Keychain> keychain) {
    return this->filter_handle_set(this->try_get_public_key(keychain), [this, &link_schema] {
        return this->backend->query_for_pattern(link_schema);
    });
}

shared_ptr<atomdb_api_types::HandleList> ProtectedAtomDB::query_for_targets(
    const string& handle, shared_ptr<Keychain> keychain) {
    if (!this->authorize_read(handle, keychain)) {
        return make_shared<atomdb_api_types::HandleListInMemory>();
    }
    return this->backend->query_for_targets(handle);
}

shared_ptr<atomdb_api_types::HandleSet> ProtectedAtomDB::query_for_incoming_set(
    const string& handle, shared_ptr<Keychain> keychain) {
    return this->filter_handle_set(this->authorize_read(handle, keychain), [this, &handle] {
        return this->backend->query_for_incoming_set(handle);
    });
}

bool ProtectedAtomDB::atom_exists(const string& handle, shared_ptr<Keychain> keychain) {
    return this->backend->atom_exists(handle) ? this->authorize_read(handle, keychain).has_value()
                                              : false;
}

bool ProtectedAtomDB::node_exists(const string& handle, shared_ptr<Keychain> keychain) {
    return this->backend->node_exists(handle) ? this->authorize_read(handle, keychain).has_value()
                                              : false;
}

bool ProtectedAtomDB::link_exists(const string& handle, shared_ptr<Keychain> keychain) {
    return this->backend->link_exists(handle) ? this->authorize_read(handle, keychain).has_value()
                                              : false;
}

set<string> ProtectedAtomDB::atoms_exist(const vector<string>& handles, shared_ptr<Keychain> keychain) {
    return this->check_handles(keychain,
                               [this, &handles] { return this->backend->atoms_exist(handles); });
}

set<string> ProtectedAtomDB::nodes_exist(const vector<string>& handles, shared_ptr<Keychain> keychain) {
    return this->check_handles(keychain,
                               [this, &handles] { return this->backend->nodes_exist(handles); });
}

set<string> ProtectedAtomDB::links_exist(const vector<string>& handles, shared_ptr<Keychain> keychain) {
    return this->check_handles(keychain,
                               [this, &handles] { return this->backend->links_exist(handles); });
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

shared_ptr<Atom> ProtectedAtomDB::get_atom(const string& handle) { raise_keychain_required("get_atom"); }

shared_ptr<Node> ProtectedAtomDB::get_node(const string& handle) { raise_keychain_required("get_node"); }

shared_ptr<Link> ProtectedAtomDB::get_link(const string& handle) { raise_keychain_required("get_link"); }

vector<shared_ptr<Atom>> ProtectedAtomDB::get_matching_atoms(bool is_toplevel, Atom& key) {
    raise_keychain_required("get_matching_atoms");
}

shared_ptr<atomdb_api_types::HandleSet> ProtectedAtomDB::query_for_pattern(
    const LinkSchema& link_schema) {
    raise_keychain_required("query_for_pattern");
}

shared_ptr<atomdb_api_types::HandleList> ProtectedAtomDB::query_for_targets(const string& handle) {
    raise_keychain_required("query_for_targets");
}

shared_ptr<atomdb_api_types::HandleSet> ProtectedAtomDB::query_for_incoming_set(const string& handle) {
    raise_keychain_required("query_for_incoming_set");
}

bool ProtectedAtomDB::atom_exists(const string& handle) { raise_keychain_required("atom_exists"); }

bool ProtectedAtomDB::node_exists(const string& handle) { raise_keychain_required("node_exists"); }

bool ProtectedAtomDB::link_exists(const string& handle) { raise_keychain_required("link_exists"); }

set<string> ProtectedAtomDB::atoms_exist(const vector<string>& handles) {
    raise_keychain_required("atoms_exist");
}

set<string> ProtectedAtomDB::nodes_exist(const vector<string>& handles) {
    raise_keychain_required("nodes_exist");
}

set<string> ProtectedAtomDB::links_exist(const vector<string>& handles) {
    raise_keychain_required("links_exist");
}

string ProtectedAtomDB::add_atom(const atoms::Atom* atom, const atoms::Merger* merger) {
    raise_keychain_required("add_atom");
}

string ProtectedAtomDB::add_node(const atoms::Node* node, const atoms::Merger* merger) {
    raise_keychain_required("add_node");
}

string ProtectedAtomDB::add_link(const atoms::Link* link, const atoms::Merger* merger) {
    raise_keychain_required("add_link");
}

vector<string> ProtectedAtomDB::add_atoms(const vector<atoms::Atom*>& atom_list,
                                          bool is_transactional,
                                          const atoms::Merger* merger) {
    raise_keychain_required("add_atoms");
}

vector<string> ProtectedAtomDB::add_nodes(const vector<atoms::Node*>& nodes,
                                          bool is_transactional,
                                          const atoms::Merger* merger) {
    raise_keychain_required("add_nodes");
}

vector<string> ProtectedAtomDB::add_links(const vector<atoms::Link*>& links,
                                          bool is_transactional,
                                          const atoms::Merger* merger) {
    raise_keychain_required("add_links");
}

bool ProtectedAtomDB::delete_atom(const string& handle, bool delete_link_targets) {
    raise_keychain_required("delete_atom");
}

bool ProtectedAtomDB::delete_node(const string& handle, bool delete_link_targets) {
    raise_keychain_required("delete_node");
}

bool ProtectedAtomDB::delete_link(const string& handle, bool delete_link_targets) {
    raise_keychain_required("delete_link");
}

uint ProtectedAtomDB::delete_atoms(const vector<string>& handles, bool delete_link_targets) {
    raise_keychain_required("delete_atoms");
}

uint ProtectedAtomDB::delete_nodes(const vector<string>& handles, bool delete_link_targets) {
    raise_keychain_required("delete_nodes");
}

uint ProtectedAtomDB::delete_links(const vector<string>& handles, bool delete_link_targets) {
    raise_keychain_required("delete_links");
}

void ProtectedAtomDB::re_index_patterns(bool flush_patterns) {
    raise_keychain_required("re_index_patterns");
}

size_t ProtectedAtomDB::node_count() const { raise_keychain_required("node_count"); }

size_t ProtectedAtomDB::link_count() const { raise_keychain_required("link_count"); }

size_t ProtectedAtomDB::atom_count() const { raise_keychain_required("atom_count"); }

// --------------------------------------------------------------------------------
// Private methods

void ProtectedAtomDB::raise_keychain_required(const string& method_name) {
    RAISE_ERROR("ProtectedAtomDB::" + method_name +
                "() is unavailable in protected AtomDBs. Use the public API in ProtectedAtomDB passing "
                "a Keychain.");
}

optional<string> ProtectedAtomDB::try_get_public_key(const shared_ptr<Keychain>& keychain) {
    string public_key = keychain ? keychain->get_public_key(this->uid_) : "";
    if (!this->manifest->ensure_profile_loaded(public_key)) return nullopt;
    return public_key;
}

optional<string> ProtectedAtomDB::authorize_read(const string& handle,
                                                 const shared_ptr<Keychain>& keychain) {
    auto public_key = this->try_get_public_key(keychain);
    if (!public_key || !this->can_read(handle, *public_key)) return nullopt;
    return public_key;
}

optional<string> ProtectedAtomDB::authorize_read(const shared_ptr<Atom>& atom,
                                                 const shared_ptr<Keychain>& keychain) {
    auto public_key = this->try_get_public_key(keychain);
    if (!public_key || !this->can_read(atom, *public_key)) return nullopt;
    return public_key;
}

set<string> ProtectedAtomDB::check_handles(shared_ptr<Keychain> keychain,
                                           const function<set<string>()>& exists_many) {
    auto public_key = this->try_get_public_key(keychain);
    if (!public_key) return {};
    return this->filter_handles(exists_many(), *public_key);
}

shared_ptr<atomdb_api_types::HandleSet> ProtectedAtomDB::filter_handle_set(
    const optional<string>& public_key,
    const function<shared_ptr<atomdb_api_types::HandleSet>()>& query) {
    if (!public_key) return make_shared<atomdb_api_types::HandleSetInMemory>();

    auto original_handle_set = query();

    auto authorized_handle_set = make_shared<atomdb_api_types::HandleSetInMemory>();

    if (original_handle_set == nullptr) {
        return authorized_handle_set;
    }

    auto it = original_handle_set->get_iterator();

    while (true) {
        char* handle_cstr = it->next();
        if (!handle_cstr) break;
        string handle(handle_cstr);

        if (this->can_read(handle, *public_key)) {
            authorized_handle_set->add_handle(
                handle,
                original_handle_set->get_metta_expressions_by_handle(handle),
                original_handle_set->get_assignments_by_handle(handle));
        }
    }

    LOG_DEBUG("ProtectedAtomDB::filter_handle_set() - original handle_set: " +
              std::to_string(original_handle_set->size()));
    LOG_DEBUG("ProtectedAtomDB::filter_handle_set() - authorized handle_set: " +
              std::to_string(authorized_handle_set->size()));

    return authorized_handle_set;
}

set<string> ProtectedAtomDB::filter_handles(const set<string>& original_handles,
                                            const string& public_key) {
    set<string> authorized_handles;
    for (const auto& handle : original_handles) {
        if (this->can_read(handle, public_key)) {
            authorized_handles.insert(handle);
        }
    }
    return authorized_handles;
}

vector<shared_ptr<Atom>> ProtectedAtomDB::filter_atoms(const vector<shared_ptr<Atom>>& original_atoms,
                                                       const string& public_key) {
    vector<shared_ptr<Atom>> authorized_atoms;
    for (const auto& atom : original_atoms) {
        if (this->can_read(atom, public_key)) {
            authorized_atoms.push_back(atom);
        }
    }
    return authorized_atoms;
}
