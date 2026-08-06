#include "InMemoryDB.h"

#include <algorithm>
#include <memory>

#include "Hasher.h"
#include "InMemoryDBAPITypes.h"
#include "Link.h"
#include "LinkSchema.h"
#include "Merger.h"
#include "Node.h"
#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace atomdb;
using namespace atomdb_api_types;
using namespace atoms;
using namespace commons;

// Helper class to wrap Atom in HandleTrie
class AtomTrieValue : public HandleTrie::TrieValue {
   public:
    AtomTrieValue(Atom* atom) : atom_(atom) {}
    ~AtomTrieValue() override { delete atom_; }
    void merge(HandleTrie::TrieValue* other) override {
        // For now, just replace (could be enhanced later)
        delete atom_;
        atom_ = dynamic_cast<AtomTrieValue*>(other)->atom_;
        dynamic_cast<AtomTrieValue*>(other)->atom_ = NULL;  // Prevent double delete
    }
    Atom* get_atom() { return atom_; }

   private:
    Atom* atom_;
};

// Helper class to store sets of atom handles in HandleTrie for pattern/incoming set indexing
class HandleSetTrieValue : public HandleTrie::TrieValue {
   public:
    HandleSetTrieValue() {}
    ~HandleSetTrieValue() override {}
    void merge(HandleTrie::TrieValue* other) override {
        // Merge sets when the same handle is inserted multiple times
        HandleSetTrieValue* other_value = dynamic_cast<HandleSetTrieValue*>(other);
        if (other_value != NULL) {
            handle_set_.insert(other_value->handle_set_.begin(), other_value->handle_set_.end());
        }
    }
    void add_handle(const string& handle) { handle_set_.insert(handle); }
    void remove_handle(const string& handle) { handle_set_.erase(handle); }
    const set<string>& get_handles() const { return handle_set_; }
    bool empty() const { return handle_set_.empty(); }

   private:
    set<string> handle_set_;
};

namespace {
shared_ptr<Atom> clone_atom(const Atom* atom) {
    if (atom == nullptr) {
        return nullptr;
    }

    if (atom->arity() == 0) {
        auto node = dynamic_cast<const Node*>(atom);
        return (node != nullptr) ? make_shared<Node>(*node) : nullptr;
    }

    auto link = dynamic_cast<const Link*>(atom);
    return (link != nullptr) ? make_shared<Link>(*link) : nullptr;
}

shared_ptr<HandleTrie> make_trie() { return make_shared<HandleTrie>(HANDLE_HASH_SIZE - 1); }
}  // namespace

InMemoryDB::InMemoryDB(const string& context)
    : context_(context),
      atoms_trie_(make_trie()),
      pattern_index_trie_(make_trie()),
      incoming_sets_trie_(make_trie()) {}

InMemoryDB::~InMemoryDB() = default;

bool InMemoryDB::allow_nested_indexing() { return false; }

shared_ptr<HandleTrie> InMemoryDB::atoms_trie() const {
    lock_guard<mutex> lock(trie_ptr_mutex_);
    return atoms_trie_;
}

shared_ptr<HandleTrie> InMemoryDB::pattern_index_trie() const {
    lock_guard<mutex> lock(trie_ptr_mutex_);
    return pattern_index_trie_;
}

shared_ptr<HandleTrie> InMemoryDB::incoming_sets_trie() const {
    lock_guard<mutex> lock(trie_ptr_mutex_);
    return incoming_sets_trie_;
}

// ---------------------------------------------------------------------------
// Reads — no write_mutex_; trie snapshots + HandleTrie's per-node locking
// ---------------------------------------------------------------------------

shared_ptr<Atom> InMemoryDB::get_atom(const string& handle) {
    auto trie_value = atoms_trie()->lookup(handle);
    if (trie_value == NULL) {
        return nullptr;
    }
    auto atom_trie_value = dynamic_cast<AtomTrieValue*>(trie_value);
    if (atom_trie_value == NULL) {
        return nullptr;
    }
    // Return a deep copy (caller must not observe internal trie-owned storage).
    return clone_atom(atom_trie_value->get_atom());
}

shared_ptr<Node> InMemoryDB::get_node(const string& handle) {
    auto atom = get_atom(handle);
    return dynamic_pointer_cast<Node>(atom);
}

shared_ptr<Link> InMemoryDB::get_link(const string& handle) {
    auto atom = get_atom(handle);
    return dynamic_pointer_cast<Link>(atom);
}

shared_ptr<HandleSet> InMemoryDB::query_for_pattern(const LinkSchema& link_schema) {
    auto handle_set = make_shared<HandleSetInMemory>();

    auto pattern_trie_value =
        dynamic_cast<HandleSetTrieValue*>(pattern_index_trie()->lookup(link_schema.handle()));
    if (pattern_trie_value != NULL) {
        for (const auto& handle : pattern_trie_value->get_handles()) {
            handle_set->add_handle(handle);
        }
    }

    return handle_set;
}

shared_ptr<HandleList> InMemoryDB::query_for_targets(const string& handle) {
    auto trie_value = atoms_trie()->lookup(handle);
    if (trie_value == NULL) {
        return nullptr;
    }
    auto atom_trie_value = dynamic_cast<AtomTrieValue*>(trie_value);
    if (atom_trie_value == NULL) {
        return nullptr;
    }
    Atom* atom = atom_trie_value->get_atom();
    if (!Atom::is_link(*atom)) {
        return nullptr;  // Not a link, so no targets
    }
    Link* link = dynamic_cast<Link*>(atom);
    return make_shared<HandleListInMemory>(link->targets);
}

shared_ptr<HandleSet> InMemoryDB::query_for_incoming_set(const string& handle) {
    auto handle_set = make_shared<HandleSetInMemory>();
    auto incoming_set_trie_value =
        dynamic_cast<HandleSetTrieValue*>(incoming_sets_trie()->lookup(handle));
    if (incoming_set_trie_value != NULL) {
        for (const auto& link_handle : incoming_set_trie_value->get_handles()) {
            handle_set->add_handle(link_handle);
        }
    }
    return handle_set;
}

vector<shared_ptr<Atom>> InMemoryDB::get_matching_atoms(bool is_toplevel, Atom& key) {
    vector<shared_ptr<Atom>> matching_atoms;
    auto trie_value = atoms_trie()->lookup(key.handle());
    if (trie_value == NULL) {
        return matching_atoms;
    }
    auto atom_trie_value = dynamic_cast<AtomTrieValue*>(trie_value);
    if (atom_trie_value == NULL) {
        return matching_atoms;
    }
    auto cloned = clone_atom(atom_trie_value->get_atom());
    if (cloned != nullptr) {
        matching_atoms.push_back(cloned);
    }
    return matching_atoms;
}

bool InMemoryDB::atom_exists(const string& handle) { return atoms_trie()->lookup(handle) != NULL; }

bool InMemoryDB::node_exists(const string& handle) {
    auto trie_value = atoms_trie()->lookup(handle);
    if (trie_value == NULL) {
        return false;
    }
    auto atom_trie_value = dynamic_cast<AtomTrieValue*>(trie_value);
    if (atom_trie_value == NULL) {
        return false;
    }
    Atom* atom = atom_trie_value->get_atom();
    return Atom::is_node(*atom);
}

bool InMemoryDB::link_exists(const string& handle) {
    auto trie_value = atoms_trie()->lookup(handle);
    if (trie_value == NULL) {
        return false;
    }
    auto atom_trie_value = dynamic_cast<AtomTrieValue*>(trie_value);
    if (atom_trie_value == NULL) {
        return false;
    }
    Atom* atom = atom_trie_value->get_atom();
    return Atom::is_link(*atom);
}

set<string> InMemoryDB::atoms_exist(const vector<string>& handles) {
    set<string> existing;
    auto trie = atoms_trie();
    for (const auto& handle : handles) {
        if (trie->lookup(handle) != NULL) {
            existing.insert(handle);
        }
    }
    return existing;
}

set<string> InMemoryDB::nodes_exist(const vector<string>& handles) {
    set<string> existing;
    for (const auto& handle : handles) {
        if (this->node_exists(handle)) {
            existing.insert(handle);
        }
    }
    return existing;
}

set<string> InMemoryDB::links_exist(const vector<string>& handles) {
    set<string> existing;
    for (const auto& handle : handles) {
        if (this->link_exists(handle)) {
            existing.insert(handle);
        }
    }
    return existing;
}

size_t InMemoryDB::node_count() const { RAISE_ERROR("node_count() is not implemented yet"); }

size_t InMemoryDB::link_count() const { RAISE_ERROR("link_count() is not implemented yet"); }

size_t InMemoryDB::atom_count() const { return static_cast<size_t>(atoms_trie()->size()); }

vector<shared_ptr<Atom>> InMemoryDB::get_all_atoms() {
    vector<shared_ptr<Atom>> atoms;
    auto trie = atoms_trie();
    atoms.reserve(trie->size());
    trie->traverse(
        false,
        [](HandleTrie::TrieNode* node, void* data) -> bool {
            if (node->value == nullptr) {
                return false;
            }
            auto atom_trie_value = dynamic_cast<AtomTrieValue*>(node->value);
            if (atom_trie_value == nullptr) {
                return false;
            }
            auto* out = static_cast<vector<shared_ptr<Atom>>*>(data);
            auto cloned = clone_atom(atom_trie_value->get_atom());
            if (cloned != nullptr) {
                out->push_back(cloned);
            }
            return false;
        },
        &atoms);
    return atoms;
}

// ---------------------------------------------------------------------------
// Mutations — serialized by write_mutex_
// ---------------------------------------------------------------------------

string InMemoryDB::add_atom(const atoms::Atom* atom, const atoms::Merger* merger) {
    if (atom->arity() == 0) {
        return add_node(dynamic_cast<const atoms::Node*>(atom), merger);
    } else {
        return add_link(dynamic_cast<const atoms::Link*>(atom), merger);
    }
}

string InMemoryDB::add_node(const atoms::Node* node, const atoms::Merger* merger) {
    lock_guard<mutex> lock(write_mutex_);
    return add_node_unlocked(node, merger);
}

string InMemoryDB::add_node_unlocked(const atoms::Node* node, const atoms::Merger* merger) {
    string handle = node->handle();
    auto trie = atoms_trie();

    auto existing = trie->lookup(handle);
    if ((existing == NULL) || (merger == NULL)) {
        // Insert or upsert/replace — HandleTrie insert calls AtomTrieValue::merge,
        // which deletes the previous Atom (if any) and takes ownership of the new one.
        Node* cloned_node = new Node(*node);
        trie->insert(handle, new AtomTrieValue(cloned_node));
        return handle;
    }

    // Merge a copy; persist only when merge() returns true.
    auto* atom_trie_value = dynamic_cast<AtomTrieValue*>(existing);
    unique_ptr<Node> working(new Node(*dynamic_cast<Node*>(atom_trie_value->get_atom())));
    if (!merger->merge(working.get(), node)) {
        return "";
    }
    trie->insert(handle, new AtomTrieValue(working.release()));

    return handle;
}

string InMemoryDB::add_link(const atoms::Link* link, const atoms::Merger* merger) {
    vector<Link*> links = {const_cast<atoms::Link*>(link)};
    auto handles = this->add_links(links, false, merger);
    return handles.empty() ? "" : handles[0];
}

vector<string> InMemoryDB::add_atoms(const vector<atoms::Atom*>& atom_list,
                                     bool is_transactional,
                                     const atoms::Merger* merger) {
    if (atom_list.empty()) {
        return {};
    }
    lock_guard<mutex> lock(write_mutex_);

    vector<Node*> nodes;
    vector<Link*> links;
    for (const auto& atom : atom_list) {
        LOG_DEBUG("Adding atom: " + atom->to_string());
        if (atom->arity() == 0) {
            nodes.push_back(dynamic_cast<atoms::Node*>(atom));
        } else {
            links.push_back(dynamic_cast<atoms::Link*>(atom));
        }
    }
    auto node_handles = this->add_nodes_unlocked(nodes, is_transactional, merger);
    auto link_handles = this->add_links_unlocked(links, is_transactional, merger);

    node_handles.insert(node_handles.end(), link_handles.begin(), link_handles.end());
    return node_handles;
}

vector<string> InMemoryDB::add_nodes(const vector<atoms::Node*>& nodes,
                                     bool is_transactional,
                                     const atoms::Merger* merger) {
    if (nodes.empty()) {
        return {};
    }
    lock_guard<mutex> lock(write_mutex_);
    return add_nodes_unlocked(nodes, is_transactional, merger);
}

vector<string> InMemoryDB::add_nodes_unlocked(const vector<atoms::Node*>& nodes,
                                              bool /*is_transactional*/,
                                              const atoms::Merger* merger) {
    vector<string> handles;
    handles.reserve(nodes.size());
    for (const auto& node : nodes) {
        handles.push_back(this->add_node_unlocked(node, merger));
    }
    return handles;
}

vector<string> InMemoryDB::add_links(const vector<atoms::Link*>& links,
                                     bool is_transactional,
                                     const atoms::Merger* merger) {
    if (links.empty()) {
        return {};
    }
    lock_guard<mutex> lock(write_mutex_);
    return add_links_unlocked(links, is_transactional, merger);
}

vector<string> InMemoryDB::add_links_unlocked(const vector<atoms::Link*>& links,
                                              bool /*is_transactional*/,
                                              const atoms::Merger* merger) {
    vector<string> handles;
    handles.reserve(links.size());
    auto trie = atoms_trie();
    auto pattern_trie = pattern_index_trie();

    for (const auto& link : links) {
        string link_handle = link->handle();

        auto existing = trie->lookup(link_handle);
        bool is_new = (existing == NULL);

        if ((existing == NULL) || (merger == NULL)) {
            // Insert or upsert/replace — AtomTrieValue::merge frees the previous Atom.
            Link* cloned_link = new Link(*link);
            trie->insert(link_handle, new AtomTrieValue(cloned_link));
        } else {
            // Merge a copy; persist only when merge() returns true.
            // On failure, skip incoming-set/pattern updates — the link was already
            // indexed by whichever add created it.
            auto* atom_trie_value = dynamic_cast<AtomTrieValue*>(existing);
            unique_ptr<Link> working(new Link(*dynamic_cast<Link*>(atom_trie_value->get_atom())));
            if (!merger->merge(working.get(), link)) {
                handles.push_back("");
                continue;
            }
            trie->insert(link_handle, new AtomTrieValue(working.release()));
        }

        // Content-addressed handles share targets, so indexes only need building on first insert.
        if (is_new) {
            for (const auto& target_handle : link->targets) {
                this->add_incoming_set_unlocked(target_handle, link_handle);
            }

            auto pattern_handles = this->match_pattern_index_schema_unlocked(link);
            for (const auto& pattern_handle : pattern_handles) {
                add_pattern_to(*pattern_trie, pattern_handle, link_handle);
            }
        }

        handles.push_back(link_handle);
    }

    return handles;
}

bool InMemoryDB::delete_atom(const string& handle, bool delete_link_targets) {
    lock_guard<mutex> lock(write_mutex_);
    return delete_atom_unlocked(handle, delete_link_targets);
}

bool InMemoryDB::delete_atom_unlocked(const string& handle, bool delete_link_targets) {
    if (this->delete_node_unlocked(handle, delete_link_targets)) {
        return true;
    }
    return this->delete_link_unlocked(handle, delete_link_targets);
}

bool InMemoryDB::delete_node(const string& handle, bool delete_link_targets) {
    lock_guard<mutex> lock(write_mutex_);
    return delete_node_unlocked(handle, delete_link_targets);
}

bool InMemoryDB::delete_node_unlocked(const string& handle, bool delete_link_targets) {
    auto trie = atoms_trie();
    auto incoming_trie = incoming_sets_trie();

    auto trie_value = trie->lookup(handle);
    if (trie_value == NULL) {
        return false;
    }
    auto atom_trie_value = dynamic_cast<AtomTrieValue*>(trie_value);
    if (atom_trie_value == NULL) {
        return false;
    }
    Atom* atom = atom_trie_value->get_atom();
    if (!Atom::is_node(*atom)) {
        return false;
    }

    vector<string> link_handles_to_delete;

    auto incoming_set_trie_value = dynamic_cast<HandleSetTrieValue*>(incoming_trie->lookup(handle));
    if (incoming_set_trie_value != NULL && !incoming_set_trie_value->empty()) {
        if (delete_link_targets) {
            link_handles_to_delete = vector<string>(incoming_set_trie_value->get_handles().begin(),
                                                    incoming_set_trie_value->get_handles().end());
        } else {
            // Cannot delete node that is referenced by links
            return false;
        }
    }

    for (const auto& link_handle : link_handles_to_delete) {
        this->delete_link_unlocked(link_handle, delete_link_targets);
    }

    trie->remove(handle);
    incoming_trie->remove(handle);

    return true;
}

bool InMemoryDB::delete_link(const string& handle, bool delete_link_targets) {
    lock_guard<mutex> lock(write_mutex_);
    return delete_link_unlocked(handle, delete_link_targets);
}

bool InMemoryDB::delete_link_unlocked(const string& handle, bool delete_link_targets) {
    auto trie = atoms_trie();
    auto incoming_trie = incoming_sets_trie();

    auto trie_value = trie->lookup(handle);
    if (trie_value == NULL) {
        return false;
    }
    auto atom_trie_value = dynamic_cast<AtomTrieValue*>(trie_value);
    if (atom_trie_value == NULL) {
        return false;
    }
    Atom* atom = atom_trie_value->get_atom();
    if (!Atom::is_link(*atom)) {
        return false;
    }

    auto link = dynamic_cast<Link*>(atom);
    auto targets = link->targets;

    vector<string> targets_to_delete;

    for (const auto& target_handle : targets) {
        this->delete_incoming_set_unlocked(target_handle, handle);

        if (delete_link_targets) {
            auto incoming_set_trie_value =
                dynamic_cast<HandleSetTrieValue*>(incoming_trie->lookup(target_handle));
            if (incoming_set_trie_value == NULL || incoming_set_trie_value->empty()) {
                targets_to_delete.push_back(target_handle);
            }
        }
    }

    vector<string> pattern_handles = this->match_pattern_index_schema_unlocked(link);
    for (const auto& pattern_handle : pattern_handles) {
        this->delete_pattern_unlocked(pattern_handle, handle);
    }

    trie->remove(handle);

    for (const auto& target_handle : targets_to_delete) {
        this->delete_atom_unlocked(target_handle, delete_link_targets);
    }

    return true;
}

uint InMemoryDB::delete_atoms(const vector<string>& handles, bool delete_link_targets) {
    lock_guard<mutex> lock(write_mutex_);
    uint deleted_count = 0;
    for (const auto& handle : handles) {
        if (this->delete_atom_unlocked(handle, delete_link_targets)) {
            deleted_count++;
        }
    }
    return deleted_count;
}

uint InMemoryDB::delete_nodes(const vector<string>& handles, bool delete_link_targets) {
    lock_guard<mutex> lock(write_mutex_);
    uint deleted_count = 0;
    for (const auto& handle : handles) {
        if (this->delete_node_unlocked(handle, delete_link_targets)) {
            deleted_count++;
        }
    }
    return deleted_count;
}

uint InMemoryDB::delete_links(const vector<string>& handles, bool delete_link_targets) {
    lock_guard<mutex> lock(write_mutex_);
    uint deleted_count = 0;
    for (const auto& handle : handles) {
        if (this->delete_link_unlocked(handle, delete_link_targets)) {
            deleted_count++;
        }
    }
    return deleted_count;
}

void InMemoryDB::drop_all() {
    lock_guard<mutex> lock(write_mutex_);
    // Swap fresh tries instead of deleting in place: concurrent readers keep their
    // pre-swap snapshots alive until they finish.
    auto fresh_atoms = make_trie();
    auto fresh_patterns = make_trie();
    auto fresh_incoming = make_trie();
    lock_guard<mutex> ptr_lock(trie_ptr_mutex_);
    atoms_trie_ = fresh_atoms;
    pattern_index_trie_ = fresh_patterns;
    incoming_sets_trie_ = fresh_incoming;
}

void InMemoryDB::re_index_patterns(bool flush_patterns) {
    lock_guard<mutex> lock(write_mutex_);

    // Build into a target trie, then publish. With flush_patterns the target is a fresh
    // trie swapped in at the end, so readers see either the old or the fully rebuilt
    // index — never a torn/deleted one.
    shared_ptr<HandleTrie> target = flush_patterns ? make_trie() : pattern_index_trie();

    struct ReIndexCtx {
        InMemoryDB* db;
        HandleTrie* target;
    } ctx{this, target.get()};

    atoms_trie()->traverse(
        false,
        [](HandleTrie::TrieNode* node, void* data) -> bool {
            auto* ctx = static_cast<ReIndexCtx*>(data);
            if (node->value == nullptr) {
                return false;
            }
            auto atom_trie_value = dynamic_cast<AtomTrieValue*>(node->value);
            if (atom_trie_value == nullptr) {
                return false;
            }
            Atom* atom = atom_trie_value->get_atom();
            if (!Atom::is_link(*atom)) {
                return false;
            }
            Link* link = dynamic_cast<Link*>(atom);
            string link_handle = link->handle();
            auto pattern_handles = ctx->db->match_pattern_index_schema_unlocked(link);
            for (const auto& pattern_handle : pattern_handles) {
                add_pattern_to(*ctx->target, pattern_handle, link_handle);
            }
            return false;
        },
        &ctx);

    if (flush_patterns) {
        lock_guard<mutex> ptr_lock(trie_ptr_mutex_);
        pattern_index_trie_ = target;
    }
}

void InMemoryDB::add_pattern(const string& pattern_handle, const string& atom_handle) {
    lock_guard<mutex> lock(write_mutex_);
    add_pattern_to(*pattern_index_trie(), pattern_handle, atom_handle);
}

vector<string> InMemoryDB::match_pattern_index_schema(const Link* link) {
    // Serialize against add_pattern_index_schema: pattern_index_schema_map is a plain
    // std::map, so unguarded concurrent read/write would be a data race.
    lock_guard<mutex> lock(write_mutex_);
    return match_pattern_index_schema_unlocked(link);
}

void InMemoryDB::add_pattern_to(HandleTrie& trie,
                                const string& pattern_handle,
                                const string& atom_handle) {
    auto pattern_trie_value = dynamic_cast<HandleSetTrieValue*>(trie.lookup(pattern_handle));
    if (pattern_trie_value == NULL) {
        pattern_trie_value = new HandleSetTrieValue();
        pattern_trie_value->add_handle(atom_handle);
        trie.insert(pattern_handle, pattern_trie_value);
    } else {
        pattern_trie_value->add_handle(atom_handle);
    }
}

void InMemoryDB::delete_pattern_unlocked(const string& pattern_handle, const string& atom_handle) {
    auto trie = pattern_index_trie();
    auto pattern_trie_value = dynamic_cast<HandleSetTrieValue*>(trie->lookup(pattern_handle));
    if (pattern_trie_value != NULL) {
        pattern_trie_value->remove_handle(atom_handle);
        if (pattern_trie_value->empty()) {
            trie->remove(pattern_handle);
        }
    }
}

void InMemoryDB::add_incoming_set_unlocked(const string& target_handle, const string& link_handle) {
    auto trie = incoming_sets_trie();
    auto incoming_set_trie_value = dynamic_cast<HandleSetTrieValue*>(trie->lookup(target_handle));
    if (incoming_set_trie_value == NULL) {
        incoming_set_trie_value = new HandleSetTrieValue();
        incoming_set_trie_value->add_handle(link_handle);
        trie->insert(target_handle, incoming_set_trie_value);
    } else {
        incoming_set_trie_value->add_handle(link_handle);
    }
}

void InMemoryDB::delete_incoming_set_unlocked(const string& target_handle, const string& link_handle) {
    auto trie = incoming_sets_trie();
    auto incoming_set_trie_value = dynamic_cast<HandleSetTrieValue*>(trie->lookup(target_handle));
    if (incoming_set_trie_value != NULL) {
        incoming_set_trie_value->remove_handle(link_handle);
        if (incoming_set_trie_value->empty()) {
            trie->remove(target_handle);
        }
    }
}

void InMemoryDB::add_pattern_index_schema(const string& tokens,
                                          const vector<vector<string>>& index_entries) {
    lock_guard<mutex> lock(write_mutex_);
    auto tokens_vector = Utils::split(tokens, ' ');
    LinkSchema link_schema(tokens_vector);

    this->pattern_index_schema_map[this->pattern_index_schema_next_priority] =
        make_tuple(move(tokens_vector), index_entries);
    this->pattern_index_schema_next_priority++;
}

vector<string> InMemoryDB::match_pattern_index_schema_unlocked(const Link* link) {
    vector<string> pattern_handles;

    const auto& map_ref = this->pattern_index_schema_map;

    const map<int, tuple<vector<string>, vector<vector<string>>>>* iter_map = &map_ref;

    map<int, tuple<vector<string>, vector<vector<string>>>> default_map;
    if (map_ref.empty()) {
        vector<string> tokens = {"LINK_TEMPLATE", "Expression", to_string(link->arity())};
        for (unsigned int i = 0; i < link->arity(); i++) {
            tokens.push_back("VARIABLE");
            tokens.push_back("v" + to_string(i + 1));
        }
        auto index_entries = this->index_entries_combinations(link->arity());
        default_map = {{1, make_tuple(move(tokens), move(index_entries))}};
        iter_map = &default_map;
    }

    vector<int> sorted_keys;
    for (const auto& pair : *iter_map) {
        sorted_keys.push_back(pair.first);
    }
    std::sort(sorted_keys.begin(), sorted_keys.end(), std::greater<int>());

    for (const auto& priority : sorted_keys) {
        const auto& value = (*iter_map).at(priority);
        auto link_schema = LinkSchema(get<0>(value));
        auto index_entries = get<1>(value);
        Assignment assignment;
        // LinkSchema::match may call back into get_atom on this DB — reads take no
        // write_mutex_, so no recursion problem.
        bool match = link_schema.match(*(Link*) link, assignment, *this);
        if (match) {
            for (const auto& index_entry : index_entries) {
                size_t index = 0;
                vector<string> hash_entries;
                for (const auto& token : index_entry) {
                    if (token == "_") {
                        hash_entries.push_back(link->targets[index]);
                    } else if (token == "*") {
                        hash_entries.push_back(Atom::WILDCARD_STRING);
                    } else {
                        string assignment_value = assignment.get(token);
                        if (assignment_value == "") {
                            RAISE_ERROR("LinkSchema assignments don't have variable: " + token);
                        }
                        hash_entries.push_back(assignment_value);
                    }
                    index++;
                }
                string hash = Hasher::link_handle(link->type, hash_entries);
                pattern_handles.push_back(hash);
            }
            break;
        }
    }
    return pattern_handles;
}

vector<vector<string>> InMemoryDB::index_entries_combinations(unsigned int arity) {
    vector<vector<string>> index_entries;
    unsigned int total = 1 << arity;  // 2^arity

    // Skip mask == 0 (all concrete): identical to the link's own handle; no separate pattern index.
    for (unsigned int mask = 1; mask < total; ++mask) {
        vector<string> index_entry;
        for (unsigned int i = 0; i < arity; ++i) {
            if (mask & (1 << i))
                index_entry.push_back("*");
            else
                index_entry.push_back("v" + to_string(i + 1));
        }
        index_entries.push_back(index_entry);
    }

    return index_entries;
}
