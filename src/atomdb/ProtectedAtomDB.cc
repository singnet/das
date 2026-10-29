#define LOG_LEVEL DEBUG_LEVEL
#include "ProtectedAtomDB.h"

#include "Atom.h"
#include "InMemoryDBAPITypes.h"
#include "Link.h"
#include "Logger.h"
#include "Node.h"
#include "RedisMongoDBAPITypes.h"
#include "Utils.h"

using namespace std;
using namespace atomdb;
using namespace commons;

namespace {
thread_local StopWatch g_can_read_unrestricted;
thread_local StopWatch g_can_read_exists;
thread_local StopWatch g_can_read_get_atom;
thread_local StopWatch g_can_read_is_granted;
thread_local unsigned long g_can_read_calls = 0;

void reset_can_read_timing() {
    g_can_read_unrestricted.reset();
    g_can_read_exists.reset();
    g_can_read_get_atom.reset();
    g_can_read_is_granted.reset();
    g_can_read_calls = 0;
}
}  // namespace

// --------------------------------------------------------------------------------
// Constructors and destructors

ProtectedAtomDB::ProtectedAtomDB(shared_ptr<AtomDB> backend) : backend(backend) {
    if (this->backend == nullptr) {
        RAISE_ERROR("ProtectedAtomDB requires a non-null backend AtomDB");
    }
    this->uid_ = this->backend->get_uid();
    this->manifest = make_shared<AuthorizationManifest>(backend);
    LOG_INFO("ProtectedAtomDB initialized (uid=" + this->uid_ + ")");
}

ProtectedAtomDB::ProtectedAtomDB(shared_ptr<AtomDB> backend, shared_ptr<AuthorizationManifest> manifest)
    : backend(backend), manifest(manifest) {
    if (this->backend == nullptr || manifest == nullptr) {
        RAISE_ERROR("ProtectedAtomDB requires an AtomDB and a non-null AuthorizationManifest.");
    }
    this->uid_ = this->backend->get_uid();
    LOG_INFO("ProtectedAtomDB initialized (uid=" + this->uid_ + ")");
}

shared_ptr<ProtectedAtomDB> ProtectedAtomDB::wrap(shared_ptr<AtomDB> atomdb) {
    return make_shared<ProtectedAtomDB>(atomdb, this->manifest);
}

// --------------------------------------------------------------------------------
// Public methods

shared_ptr<Atom> ProtectedAtomDB::get_atom(const string& handle, shared_ptr<Keychain> keychain) {
    StopWatch timer_backend;
    StopWatch timer_auth;

    timer_backend.start();
    auto atom = this->backend->get_atom(handle);
    timer_backend.stop();

    timer_auth.start();
    bool granted = this->authorize_read(atom, keychain).has_value();
    timer_auth.stop();

    // LOG_DEBUG("[ProtectedAtomDB(" << this->uid_ << ")] get_atom(" << handle << ") "
    //                               << (granted ? "granted" : "denied")
    //                               << " | backend: " << timer_backend.milliseconds() << "ms"
    //                               << " | auth: " << timer_auth.milliseconds() << "ms");
    return granted ? atom : nullptr;
}

shared_ptr<Node> ProtectedAtomDB::get_node(const string& handle, shared_ptr<Keychain> keychain) {
    return dynamic_pointer_cast<Node>(this->get_atom(handle, keychain));
}

shared_ptr<Link> ProtectedAtomDB::get_link(const string& handle, shared_ptr<Keychain> keychain) {
    return dynamic_pointer_cast<Link>(this->get_atom(handle, keychain));
}

vector<shared_ptr<Atom>> ProtectedAtomDB::get_atoms(const vector<string>& handles,
                                                    shared_ptr<Keychain> keychain) {
    vector<shared_ptr<Atom>> granted;
    auto atoms = this->backend->get_atoms(handles);
    granted.reserve(atoms.size());
    for (const auto& atom : atoms) {
        if (this->authorize_read(atom, keychain)) {
            granted.push_back(atom);
        }
    }
    return granted;
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
    auto public_key = this->try_get_public_key(keychain);
    if (!public_key) {
        LOG_DEBUG("[ProtectedAtomDB(" << this->uid_ << ")] query_for_pattern: skipped, no public key");
        return make_shared<atomdb_api_types::HandleSetInMemory>();
    }

    StopWatch timer_total;
    StopWatch timer_backend;
    timer_total.start();
    timer_backend.start();
    auto original_handle_set = this->backend->query_for_pattern(link_schema);
    timer_backend.stop();

    if (original_handle_set == nullptr) {
        timer_total.stop();
        LOG_DEBUG("[ProtectedAtomDB(" << this->uid_ << ")] query_for_pattern: backend returned nullptr"
                                      << " | backend: " << timer_backend.milliseconds() << "ms"
                                      << " | leftover: 0ms"
                                      << " | total: " << timer_total.milliseconds() << "ms");
        return make_shared<atomdb_api_types::HandleSetInMemory>();
    }

    StopWatch timer_next;
    StopWatch timer_get_atom;
    StopWatch timer_can_read;
    StopWatch timer_match;
    StopWatch timer_add;

    auto authorized_handle_set = make_shared<atomdb_api_types::HandleSetInMemory>();
    auto it = original_handle_set->get_iterator();
    unsigned long scanned = 0;

    vector<string> handles;
    
    while (true) {
        timer_next.start();
        auto redis_it = dynamic_pointer_cast<atomdb_api_types::HandleSetRedisIterator>(it);
        char* handle_cstr = redis_it ? redis_it->next_candidate() : it->next();
        timer_next.stop();
        scanned++;
        if (!handle_cstr) break;
        string handle(handle_cstr);
        handles.push_back(handle);
    }

    timer_get_atom.start();
    auto atoms = this->backend->get_atoms(handles);
    timer_get_atom.stop();

    for (auto atom : atoms) {
        timer_can_read.start();
        bool granted = this->can_read(atom, *public_key);
        timer_can_read.stop();
        if (!granted) continue;

        auto link = dynamic_pointer_cast<Link>(atom);
        if (link == nullptr) continue;

        Assignment assignment;
        timer_match.start();
        bool matched = const_cast<LinkSchema&>(link_schema).match(*link, assignment, *this->backend);
        timer_match.stop();
        if (!matched) continue;

        timer_add.start();
        string handle = atom->handle();
        authorized_handle_set->add_handle(
            handle, original_handle_set->get_metta_expressions_by_handle(handle), assignment);
        timer_add.stop();
    }

    // --- Without optimization
    // while (true) {
    //     timer_next.start();
    //     auto redis_it = dynamic_pointer_cast<atomdb_api_types::HandleSetRedisIterator>(it);
    //     char* handle_cstr = redis_it ? redis_it->next_candidate() : it->next();
    //     timer_next.stop();
    //     if (!handle_cstr) break;
    //     scanned++;
    //     string handle(handle_cstr);

    //     timer_get_atom.start();
    //     auto atom = this->backend->get_atom(handle);
    //     timer_get_atom.stop();

    //     timer_can_read.start();
    //     bool granted = this->can_read(atom, *public_key);
    //     timer_can_read.stop();
    //     if (!granted) continue;

    //     auto link = dynamic_pointer_cast<Link>(atom);
    //     if (link == nullptr) continue;

    //     Assignment assignment;
    //     timer_match.start();
    //     bool matched = const_cast<LinkSchema&>(link_schema).match(*link, assignment, *this->backend);
    //     timer_match.stop();
    //     if (!matched) continue;

    //     timer_add.start();
    //     authorized_handle_set->add_handle(
    //         handle, original_handle_set->get_metta_expressions_by_handle(handle), assignment);
    //     timer_add.stop();
    // }

    timer_total.stop();

    unsigned long total_ms = timer_total.milliseconds();
    unsigned long backend_ms = timer_backend.milliseconds();
    unsigned long next_ms = timer_next.milliseconds();
    unsigned long get_atom_ms = timer_get_atom.milliseconds();
    unsigned long can_read_ms = timer_can_read.milliseconds();
    unsigned long match_ms = timer_match.milliseconds();
    unsigned long add_ms = timer_add.milliseconds();
    unsigned long parts_ms = backend_ms + next_ms + get_atom_ms + can_read_ms + match_ms + add_ms;
    unsigned long leftover_ms = total_ms > parts_ms ? total_ms - parts_ms : 0;
    LOG_DEBUG("[ProtectedAtomDB(" << this->uid_
                                  << ")] query_for_pattern: " << original_handle_set->size() << " -> "
                                  << authorized_handle_set->size() << " authorized"
                                  << " | scanned: " << scanned << " | backend: " << backend_ms << "ms"
                                  << " | next: " << next_ms << "ms"
                                  << " | get_atom: " << get_atom_ms << "ms"
                                  << " | can_read: " << can_read_ms << "ms"
                                  << " | match: " << match_ms << "ms"
                                  << " | add_handle: " << add_ms << "ms"
                                  << " | leftover: " << leftover_ms << "ms"
                                  << " | total: " << total_ms << "ms");

    return authorized_handle_set;
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
    return this->backend->add_node(node, merger);
}

string ProtectedAtomDB::add_link(const atoms::Link* link,
                                 shared_ptr<Keychain> keychain,
                                 const atoms::Merger* merger) {
    return this->backend->add_link(link, merger);
}

vector<string> ProtectedAtomDB::add_atoms(const vector<atoms::Atom*>& atom_list,
                                          shared_ptr<Keychain> keychain,
                                          bool is_transactional,
                                          const atoms::Merger* merger) {
    return this->backend->add_atoms(atom_list, is_transactional, merger);
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
    size_t node_count = 0;
    size_t link_count = 0;
    size_t atom_count = 0;
    this->manifest->count_matching_atoms(node_count, link_count, atom_count, keychain);
    return node_count;
}

size_t ProtectedAtomDB::link_count(shared_ptr<Keychain> keychain) const {
    size_t node_count = 0;
    size_t link_count = 0;
    size_t atom_count = 0;
    this->manifest->count_matching_atoms(node_count, link_count, atom_count, keychain);
    return link_count;
}

size_t ProtectedAtomDB::atom_count(shared_ptr<Keychain> keychain) const {
    size_t node_count = 0;
    size_t link_count = 0;
    size_t atom_count = 0;
    this->manifest->count_matching_atoms(node_count, link_count, atom_count, keychain);
    return atom_count;
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

vector<shared_ptr<Atom>> ProtectedAtomDB::get_atoms(const vector<string>& handles) {
    raise_keychain_required("get_atoms");
}

vector<shared_ptr<Node>> ProtectedAtomDB::get_nodes(const vector<string>& handles) {
    raise_keychain_required("get_nodes");
}

vector<shared_ptr<Link>> ProtectedAtomDB::get_links(const vector<string>& handles) {
    raise_keychain_required("get_links");
}

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

bool ProtectedAtomDB::can_read(const string& handle, const string& public_key) {
    if (public_key.empty()) return false;

    g_can_read_calls++;
    g_can_read_unrestricted.start();
    bool unrestricted = this->manifest->is_unrestricted(public_key);
    g_can_read_unrestricted.stop();
    if (unrestricted) {
        g_can_read_exists.start();
        bool exists = this->backend->atom_exists(handle);
        g_can_read_exists.stop();
        return exists;
    }

    g_can_read_get_atom.start();
    auto atom = this->backend->get_atom(handle);
    g_can_read_get_atom.stop();
    g_can_read_is_granted.start();
    bool granted = this->can_read(atom, public_key);
    g_can_read_is_granted.stop();
    return granted;
}

void ProtectedAtomDB::raise_keychain_required(const string& method_name) {
    RAISE_ERROR("ProtectedAtomDB::" + method_name +
                "() is unavailable in protected AtomDBs. Use the public API in ProtectedAtomDB passing "
                "a Keychain.");
}

optional<string> ProtectedAtomDB::try_get_public_key(const shared_ptr<Keychain>& keychain) {
    string public_key = keychain ? keychain->get_public_key(this->uid_) : "";
    if (public_key == "") {
        return nullopt;
    }

    StopWatch timer;
    timer.start();
    bool loaded = this->manifest->ensure_profile_loaded(public_key);
    timer.stop();
    if (!loaded) {
        LOG_DEBUG("[ProtectedAtomDB(" << this->uid_
                                      << ")] no authorization profile for public key | time: "
                                      << timer.milliseconds() << "ms");
        return nullopt;
    }
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
    if (atom == nullptr) return nullopt;
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
    if (!public_key) {
        LOG_DEBUG("[ProtectedAtomDB(" << this->uid_ << ")] filter_handle_set: skipped, no public key");
        return make_shared<atomdb_api_types::HandleSetInMemory>();
    }

    StopWatch timer_total;
    StopWatch timer_backend;
    timer_total.start();
    timer_backend.start();
    auto original_handle_set = query();
    timer_backend.stop();

    if (original_handle_set == nullptr) {
        timer_total.stop();
        LOG_DEBUG("[ProtectedAtomDB(" << this->uid_ << ")] filter_handle_set: backend returned nullptr"
                                      << " | backend: " << timer_backend.milliseconds() << "ms"
                                      << " | leftover: 0ms"
                                      << " | total: " << timer_total.milliseconds() << "ms");
        return make_shared<atomdb_api_types::HandleSetInMemory>();
    }

    if (this->manifest->is_unrestricted(*public_key)) {
        timer_total.stop();
        unsigned long total_ms = timer_total.milliseconds();
        unsigned long backend_ms = timer_backend.milliseconds();
        unsigned long leftover_ms = total_ms > backend_ms ? total_ms - backend_ms : 0;
        LOG_DEBUG("[ProtectedAtomDB(" << this->uid_
                                      << ")] filter_handle_set: unrestricted, skipping authorization"
                                      << " | backend: " << backend_ms << "ms"
                                      << " | leftover: " << leftover_ms << "ms"
                                      << " | total: " << total_ms << "ms");
        return original_handle_set;
    }

    StopWatch timer_next;
    StopWatch timer_can_read;
    StopWatch timer_add;
    reset_can_read_timing();

    auto it = original_handle_set->get_iterator();
    auto authorized_handle_set = make_shared<atomdb_api_types::HandleSetInMemory>();
    unsigned long scanned = 0;
    while (true) {
        timer_next.start();
        char* handle_cstr = it->next();
        timer_next.stop();
        if (!handle_cstr) break;
        scanned++;
        string handle(handle_cstr);

        timer_can_read.start();
        bool granted = this->can_read(handle, *public_key);
        timer_can_read.stop();
        if (granted) {
            timer_add.start();
            authorized_handle_set->add_handle(
                handle,
                original_handle_set->get_metta_expressions_by_handle(handle),
                original_handle_set->get_assignments_by_handle(handle));
            timer_add.stop();
        }
    }
    timer_total.stop();

    unsigned long total_ms = timer_total.milliseconds();
    unsigned long backend_ms = timer_backend.milliseconds();
    unsigned long next_ms = timer_next.milliseconds();
    unsigned long can_read_ms = timer_can_read.milliseconds();
    unsigned long add_ms = timer_add.milliseconds();
    unsigned long parts_ms = backend_ms + next_ms + can_read_ms + add_ms;
    unsigned long leftover_ms = total_ms > parts_ms ? total_ms - parts_ms : 0;
    unsigned long can_read_parts =
        g_can_read_unrestricted.milliseconds() + g_can_read_exists.milliseconds() +
        g_can_read_get_atom.milliseconds() + g_can_read_is_granted.milliseconds();
    unsigned long can_read_leftover = can_read_ms > can_read_parts ? can_read_ms - can_read_parts : 0;
    LOG_DEBUG("[ProtectedAtomDB(" << this->uid_
                                  << ")] filter_handle_set: " << original_handle_set->size() << " -> "
                                  << authorized_handle_set->size() << " authorized"
                                  << " | scanned: " << scanned << " | backend: " << backend_ms << "ms"
                                  << " | next: " << next_ms << "ms"
                                  << " | can_read: " << can_read_ms << "ms"
                                  << " | add_handle: " << add_ms << "ms"
                                  << " | leftover: " << leftover_ms << "ms"
                                  << " | total: " << total_ms << "ms");
    LOG_DEBUG("[ProtectedAtomDB(" << this->uid_ << ")] can_read breakdown"
                                  << " | calls: " << g_can_read_calls << " | is_unrestricted: "
                                  << g_can_read_unrestricted.milliseconds() << "ms"
                                  << " | atom_exists: " << g_can_read_exists.milliseconds() << "ms"
                                  << " | get_atom: " << g_can_read_get_atom.milliseconds() << "ms"
                                  << " | is_granted: " << g_can_read_is_granted.milliseconds() << "ms"
                                  << " | leftover: " << can_read_leftover << "ms"
                                  << " | total: " << can_read_ms << "ms");

    return authorized_handle_set;
}

set<string> ProtectedAtomDB::filter_handles(const set<string>& original_handles,
                                            const string& public_key) {
    if (this->manifest->is_unrestricted(public_key)) {
        return original_handles;
    }
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
