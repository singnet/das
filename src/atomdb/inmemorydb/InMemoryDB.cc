#include "InMemoryDB.h"

#include <algorithm>
#include <memory>

#include "Hasher.h"
#include "InMemoryDBAPITypes.h"
#include "Link.h"
#include "LinkSchema.h"
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

// Helper functions and data structures for traverse callbacks
namespace {
struct ReIndexData {
    InMemoryDB* db;
};

bool re_index_visitor(HandleTrie::TrieNode* node, void* data) {
    ReIndexData* index_data = static_cast<ReIndexData*>(data);
    if (node->value != NULL) {
        auto atom_trie_value = dynamic_cast<AtomTrieValue*>(node->value);
        if (atom_trie_value != NULL) {
            Atom* atom = atom_trie_value->get_atom();
            if (Atom::is_link(*atom)) {
                Link* link = dynamic_cast<Link*>(atom);
                string link_handle = link->handle();
                // Index patterns
                auto pattern_handles = index_data->db->match_pattern_index_schema(link);
                for (const auto& pattern_handle : pattern_handles) {
                    index_data->db->add_pattern(pattern_handle, link_handle);
                }
            }
        }
    }
    return false;  // Continue traversal
}
}  // namespace

InMemoryDB::InMemoryDB(const string& context)
    : context_(context),
      atoms_trie_(new HandleTrie(HANDLE_HASH_SIZE - 1)),
      pattern_index_trie_(new HandleTrie(HANDLE_HASH_SIZE - 1)),
      incoming_sets_trie_(new HandleTrie(HANDLE_HASH_SIZE - 1)) {}

InMemoryDB::~InMemoryDB() {
    // Traverse and delete all atoms
    this->atoms_trie_->traverse(
        false,
        [](HandleTrie::TrieNode* node, void* data) -> bool {
            if (node->value != NULL) {
                delete node->value;
                node->value = NULL;
            }
            return false;  // Continue traversal
        },
        NULL);
    delete this->atoms_trie_;

    // Traverse and delete all pattern index entries
    this->pattern_index_trie_->traverse(
        false,
        [](HandleTrie::TrieNode* node, void* data) -> bool {
            if (node->value != NULL) {
                delete node->value;
                node->value = NULL;
            }
            return false;  // Continue traversal
        },
        NULL);
    delete this->pattern_index_trie_;

    // Traverse and delete all incoming set entries
    this->incoming_sets_trie_->traverse(
        false,
        [](HandleTrie::TrieNode* node, void* data) -> bool {
            if (node->value != NULL) {
                delete node->value;
                node->value = NULL;
            }
            return false;  // Continue traversal
        },
        NULL);
    delete this->incoming_sets_trie_;
}

bool InMemoryDB::allow_nested_indexing() { return false; }

shared_ptr<Atom> InMemoryDB::get_atom(const string& handle) {
    auto trie_value = this->atoms_trie_->lookup(handle);
    if (trie_value == NULL) {
        return nullptr;
    }
    auto atom_trie_value = dynamic_cast<AtomTrieValue*>(trie_value);
    if (atom_trie_value == NULL) {
        return nullptr;
    }
    // Clone the atom to return a shared_ptr (caller doesn't own the original)
    Atom* atom = atom_trie_value->get_atom();
    if (atom->arity() == 0) {
        auto node = dynamic_cast<Node*>(atom);
        return make_shared<Node>(*node);
    } else {
        auto link = dynamic_cast<Link*>(atom);
        return make_shared<Link>(*link);
    }
}

shared_ptr<Node> InMemoryDB::get_node(const string& handle) {
    auto atom = get_atom(handle);
    if (atom != nullptr) {
        return make_shared<Node>(*dynamic_cast<Node*>(atom.get()));
    }
    return nullptr;
}

shared_ptr<Link> InMemoryDB::get_link(const string& handle) {
    auto atom = get_atom(handle);
    if (atom != nullptr) {
        return make_shared<Link>(*dynamic_cast<Link*>(atom.get()));
    }
    return nullptr;
}

shared_ptr<HandleSet> InMemoryDB::query_for_pattern(const LinkSchema& link_schema) {
    auto handle_set = make_shared<HandleSetInMemory>();

    // Check if we have this pattern indexed in the HandleTrie
    auto pattern_trie_value =
        dynamic_cast<HandleSetTrieValue*>(pattern_index_trie_->lookup(link_schema.handle()));
    if (pattern_trie_value != NULL) {
        for (const auto& handle : pattern_trie_value->get_handles()) {
            handle_set->add_handle(handle);
        }
    }

    return handle_set;
}

shared_ptr<HandleList> InMemoryDB::query_for_targets(const string& handle) {
    auto trie_value = atoms_trie_->lookup(handle);
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
        dynamic_cast<HandleSetTrieValue*>(this->incoming_sets_trie_->lookup(handle));
    if (incoming_set_trie_value != NULL) {
        for (const auto& link_handle : incoming_set_trie_value->get_handles()) {
            handle_set->add_handle(link_handle);
        }
    }
    return handle_set;
}

vector<shared_ptr<Atom>> InMemoryDB::get_matching_atoms(bool is_toplevel, Atom& key) {
    vector<shared_ptr<Atom>> matching_atoms;
    auto trie_value = atoms_trie_->lookup(key.handle());
    if (trie_value == NULL) {
        return matching_atoms;
    }
    auto atom_trie_value = dynamic_cast<AtomTrieValue*>(trie_value);
    if (atom_trie_value == NULL) {
        return matching_atoms;
    }
    Atom* atom = atom_trie_value->get_atom();
    if (Atom::is_node(*atom)) {
        matching_atoms.push_back(shared_ptr<Node>(dynamic_cast<Node*>(atom)));
    } else {
        matching_atoms.push_back(shared_ptr<Link>(dynamic_cast<Link*>(atom)));
    }
    return matching_atoms;
}

bool InMemoryDB::atom_exists(const string& handle) { return atoms_trie_->lookup(handle) != NULL; }

bool InMemoryDB::node_exists(const string& handle) {
    auto trie_value = atoms_trie_->lookup(handle);
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
    auto trie_value = atoms_trie_->lookup(handle);
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
    for (const auto& handle : handles) {
        if (atoms_trie_->lookup(handle) != NULL) {
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

string InMemoryDB::add_atom(const atoms::Atom* atom, bool throw_if_exists) {
    if (atom->arity() == 0) {
        return add_node(dynamic_cast<const atoms::Node*>(atom), throw_if_exists);
    } else {
        return add_link(dynamic_cast<const atoms::Link*>(atom), throw_if_exists);
    }
}

string InMemoryDB::add_node(const atoms::Node* node, bool throw_if_exists) {
    string handle = node->handle();

    if (throw_if_exists && this->node_exists(handle)) {
        Utils::error("Node already exists: " + handle);
        return "";
    }

    // Check if already exists
    auto existing = atoms_trie_->lookup(handle);
    if (existing != NULL && !throw_if_exists) {
        return handle;  // Already exists, return handle
    }

    // Clone the node to store in trie
    Node* cloned_node = new Node(*node);
    auto atom_trie_value = new AtomTrieValue(cloned_node);
    atoms_trie_->insert(handle, atom_trie_value);

    return handle;
}

string InMemoryDB::add_link(const atoms::Link* link, bool throw_if_exists) {
    vector<Link*> links = {const_cast<atoms::Link*>(link)};
    auto handles = this->add_links(links, throw_if_exists, false);
    return handles.empty() ? "" : handles[0];
}

vector<string> InMemoryDB::add_atoms(const vector<atoms::Atom*>& atoms,
                                     bool throw_if_exists,
                                     bool is_transactional) {
    if (atoms.empty()) {
        return {};
    }

    vector<Node*> nodes;
    vector<Link*> links;
    for (const auto& atom : atoms) {
        LOG_DEBUG("Adding atom: " + atom->to_string());
        if (atom->arity() == 0) {
            nodes.push_back(dynamic_cast<atoms::Node*>(atom));
        } else {
            links.push_back(dynamic_cast<atoms::Link*>(atom));
        }
    }
    auto node_handles = this->add_nodes(nodes, throw_if_exists, is_transactional);
    auto link_handles = this->add_links(links, throw_if_exists, is_transactional);

    node_handles.insert(node_handles.end(), link_handles.begin(), link_handles.end());
    return node_handles;
}

vector<string> InMemoryDB::add_nodes(const vector<atoms::Node*>& nodes,
                                     bool throw_if_exists,
                                     bool is_transactional) {
    if (nodes.empty()) {
        return {};
    }

    vector<string> handles;
    for (const auto& node : nodes) {
        handles.push_back(node->handle());
    }

    if (throw_if_exists) {
        auto existing_handles = this->nodes_exist(handles);
        if (!existing_handles.empty()) {
            vector<string> existing_handles_vector(existing_handles.begin(), existing_handles.end());
            Utils::error("Failed to insert nodes, some nodes already exist: " +
                         Utils::join(existing_handles_vector, ','));
            return {};
        }
    }

    for (const auto& node : nodes) {
        handles.push_back(this->add_node(node, throw_if_exists));
    }

    return handles;
}

vector<string> InMemoryDB::add_links(const vector<atoms::Link*>& links,
                                     bool throw_if_exists,
                                     bool is_transactional) {
    if (links.empty()) {
        return {};
    }

    if (throw_if_exists) {
        vector<string> handles;
        for (const auto& link : links) {
            handles.push_back(link->handle());
        }
        auto existing_handles = this->links_exist(handles);
        if (!existing_handles.empty()) {
            vector<string> existing_handles_vector(existing_handles.begin(), existing_handles.end());
            Utils::error("Failed to insert links, some links already exist: " +
                         Utils::join(existing_handles_vector, ','));
            return {};
        }
    }

    vector<string> handles;
    for (const auto& link : links) {
        string link_handle = link->handle();
        handles.push_back(link_handle);

        // Check if already exists
        auto existing = atoms_trie_->lookup(link_handle);
        if (existing == NULL || !throw_if_exists) {
            if (existing == NULL) {
                // Clone the link to store in trie
                Link* cloned_link = new Link(*link);
                auto atom_trie_value = new AtomTrieValue(cloned_link);
                atoms_trie_->insert(link_handle, atom_trie_value);
            }

            // Update incoming sets for each target
            for (const auto& target_handle : link->targets) {
                this->add_incoming_set(target_handle, link_handle);
            }

            // Index pattern
            auto pattern_handles = this->match_pattern_index_schema(link);
            for (const auto& pattern_handle : pattern_handles) {
                this->add_pattern(pattern_handle, link_handle);
            }
        }
    }

    return handles;
}

bool InMemoryDB::delete_atom(const string& handle, bool delete_link_targets) {
    if (this->delete_node(handle, delete_link_targets)) {
        return true;
    }
    return this->delete_link(handle, delete_link_targets);
}

bool InMemoryDB::delete_node(const string& handle, bool delete_link_targets) {
    auto trie_value = this->atoms_trie_->lookup(handle);
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

    // Check incoming set - if this node is referenced by links, handle accordingly
    auto incoming_set_trie_value =
        dynamic_cast<HandleSetTrieValue*>(this->incoming_sets_trie_->lookup(handle));
    if (incoming_set_trie_value != NULL && !incoming_set_trie_value->empty()) {
        if (delete_link_targets) {
            // Collect all links that reference this node (copy the handles while holding the lock)
            link_handles_to_delete = vector<string>(incoming_set_trie_value->get_handles().begin(),
                                                    incoming_set_trie_value->get_handles().end());
        } else {
            // Cannot delete node that is referenced by links
            return false;
        }
    }

    // Delete all links that reference this node
    for (const auto& link_handle : link_handles_to_delete) {
        this->delete_link(link_handle, delete_link_targets);
    }

    // Clear the value in the trie (set to NULL)
    this->atoms_trie_->remove(handle);
    this->incoming_sets_trie_->remove(handle);

    return true;
}

bool InMemoryDB::delete_link(const string& handle, bool delete_link_targets) {
    auto trie_value = atoms_trie_->lookup(handle);
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

    // Update incoming sets for each target
    for (const auto& target_handle : targets) {
        this->delete_incoming_set(target_handle, handle);

        if (delete_link_targets) {
            // Check if target has other incoming links
            auto incoming_set_trie_value =
                dynamic_cast<HandleSetTrieValue*>(this->incoming_sets_trie_->lookup(target_handle));
            if (incoming_set_trie_value == NULL || incoming_set_trie_value->empty()) {
                // No other references, mark for deletion
                targets_to_delete.push_back(target_handle);
            }
        }
    }

    // Remove from pattern index
    vector<string> pattern_handles = this->match_pattern_index_schema(link);
    for (const auto& pattern_handle : pattern_handles) {
        this->delete_pattern(pattern_handle, handle);
    }

    // Clear the value in the trie (set to NULL)
    this->atoms_trie_->remove(handle);

    // Release locks before calling delete_atom to avoid deadlock
    // Delete targets that have no other incoming links
    for (const auto& target_handle : targets_to_delete) {
        this->delete_atom(target_handle, delete_link_targets);
    }

    return true;
}

uint InMemoryDB::delete_atoms(const vector<string>& handles, bool delete_link_targets) {
    uint deleted_count = 0;
    for (const auto& handle : handles) {
        if (this->delete_atom(handle, delete_link_targets)) {
            deleted_count++;
        }
    }
    return deleted_count;
}

uint InMemoryDB::delete_nodes(const vector<string>& handles, bool delete_link_targets) {
    uint deleted_count = 0;
    for (const auto& handle : handles) {
        if (this->delete_node(handle, delete_link_targets)) {
            deleted_count++;
        }
    }
    return deleted_count;
}

uint InMemoryDB::delete_links(const vector<string>& handles, bool delete_link_targets) {
    uint deleted_count = 0;
    for (const auto& handle : handles) {
        if (this->delete_link(handle, delete_link_targets)) {
            deleted_count++;
        }
    }
    return deleted_count;
}

void InMemoryDB::re_index_patterns(bool flush_patterns) {
    if (flush_patterns) {
        // Clear all pattern index entries by deleting and recreating the trie
        this->pattern_index_trie_->traverse(
            false,
            [](HandleTrie::TrieNode* node, void* data) -> bool {
                if (node->value != NULL) {
                    delete node->value;
                    node->value = NULL;
                }
                return false;  // Continue traversal
            },
            NULL);
        delete this->pattern_index_trie_;
        this->pattern_index_trie_ = new HandleTrie(HANDLE_HASH_SIZE - 1);
    }

    // Re-index all links
    ReIndexData index_data;
    index_data.db = this;
    this->atoms_trie_->traverse(false, re_index_visitor, &index_data);
}

// Helper methods
void InMemoryDB::add_pattern(const string& pattern_handle, const string& atom_handle) {
    auto pattern_trie_value =
        dynamic_cast<HandleSetTrieValue*>(this->pattern_index_trie_->lookup(pattern_handle));
    if (pattern_trie_value == NULL) {
        // Create new HandleSetTrieValue
        pattern_trie_value = new HandleSetTrieValue();
        pattern_trie_value->add_handle(atom_handle);
        this->pattern_index_trie_->insert(pattern_handle, pattern_trie_value);
    } else {
        // Add to existing set
        pattern_trie_value->add_handle(atom_handle);
    }
}

void InMemoryDB::delete_pattern(const string& pattern_handle, const string& atom_handle) {
    auto pattern_trie_value =
        dynamic_cast<HandleSetTrieValue*>(this->pattern_index_trie_->lookup(pattern_handle));
    if (pattern_trie_value != NULL) {
        pattern_trie_value->remove_handle(atom_handle);
        if (pattern_trie_value->empty()) {
            // Remove the pattern entry from the trie
            this->pattern_index_trie_->remove(pattern_handle);
        }
    }
}

void InMemoryDB::add_incoming_set(const string& target_handle, const string& link_handle) {
    auto incoming_set_trie_value =
        dynamic_cast<HandleSetTrieValue*>(this->incoming_sets_trie_->lookup(target_handle));
    if (incoming_set_trie_value == NULL) {
        // Create new HandleSetTrieValue
        incoming_set_trie_value = new HandleSetTrieValue();
        incoming_set_trie_value->add_handle(link_handle);
        this->incoming_sets_trie_->insert(target_handle, incoming_set_trie_value);
    } else {
        // Add to existing set
        incoming_set_trie_value->add_handle(link_handle);
    }
}

void InMemoryDB::delete_incoming_set(const string& target_handle, const string& link_handle) {
    auto incoming_set_trie_value =
        dynamic_cast<HandleSetTrieValue*>(this->incoming_sets_trie_->lookup(target_handle));
    if (incoming_set_trie_value != NULL) {
        incoming_set_trie_value->remove_handle(link_handle);
        if (incoming_set_trie_value->empty()) {
            // Remove the incoming set entry from the trie
            this->incoming_sets_trie_->remove(target_handle);
        }
    }
}

void InMemoryDB::update_incoming_set(const string& target_handle, const string& link_handle) {
    this->delete_incoming_set(target_handle, link_handle);
}

void InMemoryDB::add_pattern_index_schema(const string& tokens,
                                          const vector<vector<string>>& index_entries) {
    auto tokens_vector = Utils::split(tokens, ' ');
    LinkSchema link_schema(tokens_vector);

    this->pattern_index_schema_map[this->pattern_index_schema_next_priority] =
        make_tuple(move(tokens_vector), index_entries);
    this->pattern_index_schema_next_priority++;
}

vector<string> InMemoryDB::match_pattern_index_schema(const Link* link) {
    vector<string> pattern_handles;

    const auto& map_ref = this->pattern_index_schema_map;

    const map<int, tuple<vector<string>, vector<vector<string>>>>* iter_map = &map_ref;

    // When map is empty, use a default map
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
                            Utils::error("LinkSchema assignments don't have variable: " + token);
                        }
                        hash_entries.push_back(assignment_value);
                    }
                    index++;
                }
                string hash = Hasher::link_handle(link->type, hash_entries);
                pattern_handles.push_back(hash);
            }
            // We only need to find the first match
            break;
        }
    }
    return pattern_handles;
}

// Combination of "vX" and "*" for a given arity
vector<vector<string>> InMemoryDB::index_entries_combinations(unsigned int arity) {
    vector<vector<string>> index_entries;
    unsigned int total = 1 << arity;  // 2^arity

    for (unsigned int mask = 0; mask < total; ++mask) {
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
