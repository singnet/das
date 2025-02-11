#include "ram_only.h"

#include <algorithm>

#include "exceptions.h"
#include "expression_hasher.h"
#include "patterns.h"

using namespace std;
using namespace atomdb;

// PUBLIC METHODS //////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
const string InMemoryDB::get_node_handle(const string& node_type, const string& node_name) const {
    auto node_handle = AtomDB::build_node_handle(node_type, node_name);
    if (this->db.node.find(node_handle) != this->db.node.end()) {
        return move(node_handle);
    }
    throw AtomDoesNotExist("Nonexistent atom", node_type + ":" + node_name);
}

//------------------------------------------------------------------------------
const string InMemoryDB::get_node_name(const string& node_handle) const {
    auto it = this->db.node.find(node_handle);
    if (it != this->db.node.end()) {
        return it->second->name;
    }
    throw AtomDoesNotExist("Nonexistent atom", "node_handle: " + node_handle);
}

//------------------------------------------------------------------------------
const string InMemoryDB::get_node_type(const string& node_handle) const {
    auto it = this->db.node.find(node_handle);
    if (it != this->db.node.end()) {
        return it->second->named_type;
    }
    throw AtomDoesNotExist("Nonexistent atom", "node_handle: " + node_handle);
}

//------------------------------------------------------------------------------
const StringList InMemoryDB::get_node_by_name(const string& node_type, const string& substring) const {
    auto node_type_hash = ExpressionHasher::named_type_hash(node_type);
    StringList node_handles;
    for (const auto& [key, node] : this->db.node) {
        if (node->name.find(substring) != string::npos and node_type_hash == node->composite_type_hash) {
            node_handles.emplace_back(key);
        }
    }
    return move(node_handles);
}

//------------------------------------------------------------------------------
const StringList InMemoryDB::get_atoms_by_field(
    const vector<unordered_map<string, string>>& query) const {
    throw runtime_error("Not implemented");
}

//------------------------------------------------------------------------------
const pair<const int, const AtomList> InMemoryDB::get_atoms_by_index(
    const string& index_id, const vector<map<string, string>>& query, int cursor, int chunk_size) const {
    throw runtime_error("Not implemented");
}

//------------------------------------------------------------------------------
const StringList InMemoryDB::get_atoms_by_text_field(const string& text_value,
                                                     const optional<string>& field,
                                                     const optional<string>& text_index_id) const {
    throw runtime_error("Not implemented");
}

//------------------------------------------------------------------------------
const StringList InMemoryDB::get_node_by_name_starting_with(const string& node_type,
                                                            const string& startswith) const {
    throw runtime_error("Not implemented");
}

//------------------------------------------------------------------------------
const StringList InMemoryDB::get_all_nodes_handles(const string& node_type) const {
    auto node_type_hash = ExpressionHasher::named_type_hash(node_type);
    StringList node_handles;
    for (const auto& [handle, node] : this->db.node) {
        if (node->composite_type_hash == node_type_hash) {
            node_handles.emplace_back(handle);
        }
    }
    return move(node_handles);
}

//------------------------------------------------------------------------------
const StringList InMemoryDB::get_all_nodes_names(const string& node_type) const {
    auto node_type_hash = ExpressionHasher::named_type_hash(node_type);
    StringList node_names;
    for (const auto& [_, node] : this->db.node) {
        if (node->composite_type_hash == node_type_hash) {
            node_names.emplace_back(node->name);
        }
    }
    return move(node_names);
}

//------------------------------------------------------------------------------
const StringUnorderedSet InMemoryDB::get_all_links(const string& link_type) const {
    StringUnorderedSet link_handles;
    for (const auto& [_, link] : this->db.link) {
        if (link->named_type == link_type) {
            link_handles.emplace(link->_id);
        }
    }
    return move(link_handles);
}

//------------------------------------------------------------------------------
const string InMemoryDB::get_link_handle(const string& link_type,
                                         const StringList& target_handles) const {
    auto link_handle = AtomDB::build_link_handle(link_type, target_handles);
    if (this->db.link.find(link_handle) != this->db.link.end()) {
        return move(link_handle);
    }
    string target_handles_str = "[";
    if (not target_handles.empty()) {
        for (const auto& target_handle : target_handles) {
            target_handles_str += target_handle + ", ";
        }
        target_handles_str.pop_back();
        target_handles_str.pop_back();
    }
    target_handles_str += "]";
    throw AtomDoesNotExist("Nonexistent atom", link_type + ":" + target_handles_str);
}

//------------------------------------------------------------------------------
const string InMemoryDB::get_link_type(const string& link_handle) const {
    auto link = this->_get_link(link_handle);
    if (link) {
        return link->named_type;
    }
    throw AtomDoesNotExist("Nonexistent atom", "link_handle: " + link_handle);
}

//------------------------------------------------------------------------------
const StringList InMemoryDB::get_link_targets(const string& link_handle) const {
    auto it = this->db.outgoing_set.find(link_handle);
    if (it != this->db.outgoing_set.end()) {
        return it->second;
    }
    throw AtomDoesNotExist("Nonexistent atom", "link_handle: " + link_handle);
}

//------------------------------------------------------------------------------
const StringList InMemoryDB::get_incoming_links_handles(const string& atom_handle,
                                                        const KwArgs& kwargs) const {
    if (not kwargs.handles_only) {
        throw runtime_error(
            "'handles_only' is not true in kwargs - "
            "'InMemoryDB::get_incoming_links_atoms' should be used instead");
    }
    auto it = this->db.incoming_set.find(atom_handle);
    if (it != this->db.incoming_set.end()) {
        return move(StringList(it->second.begin(), it->second.end()));
    }
    return {};
}

//------------------------------------------------------------------------------
const vector<shared_ptr<const Atom>> InMemoryDB::get_incoming_links_atoms(const string& atom_handle,
                                                                          const KwArgs& kwargs) const {
    if (kwargs.handles_only) {
        throw runtime_error(
            "'handles_only' is true in kwargs - "
            "'InMemoryDB::get_incoming_links_handles' should be used instead");
    }
    auto it = this->db.incoming_set.find(atom_handle);
    if (it != this->db.incoming_set.end()) {
        vector<shared_ptr<const Atom>> atoms;
        atoms.reserve(it->second.size());
        for (const auto& link_handle : it->second) {
            atoms.emplace_back(this->get_atom(link_handle, kwargs));
        }
        return move(atoms);
    }
    return {};
}

//------------------------------------------------------------------------------
const StringUnorderedSet InMemoryDB::get_matched_links(const string& link_type,
                                                       const StringList& target_handles,
                                                       const KwArgs& kwargs) const {
    if (link_type != WILDCARD and
        find(target_handles.begin(), target_handles.end(), WILDCARD) == target_handles.end()) {
        try {
            return {this->get_link_handle(link_type, target_handles)};
        } catch (const AtomDoesNotExist&) {
            return {};
        }
    }

    auto link_type_hash =
        link_type == WILDCARD ? WILDCARD : ExpressionHasher::named_type_hash(link_type);

    auto handles = StringList({link_type_hash});
    handles.insert(handles.end(), target_handles.begin(), target_handles.end());

    StringUnorderedSet patterns_matched;
    auto it = this->db.patterns.find(ExpressionHasher::composite_hash(handles));
    if (it != this->db.patterns.end()) {
        patterns_matched.reserve(it->second.size());
        patterns_matched.insert(it->second.begin(), it->second.end());
    }

    if (kwargs.toplevel_only) {
        return this->_filter_non_toplevel(patterns_matched);
    }

    return move(patterns_matched);
}

//------------------------------------------------------------------------------
const StringUnorderedSet InMemoryDB::get_matched_type_template(const StringList& _template,
                                                               const KwArgs& kwargs) const {
    auto hash_base = this->_build_named_type_hash_template(_template);
    auto template_hash = ExpressionHasher::composite_hash(hash_base);
    auto it = this->db.templates.find(template_hash);
    if (it != this->db.templates.end()) {
        if (kwargs.toplevel_only) {
            return this->_filter_non_toplevel(it->second);
        }
        return it->second;
    }
    return {};
}

//------------------------------------------------------------------------------
const StringUnorderedSet InMemoryDB::get_matched_type(const string& link_type,
                                                      const KwArgs& kwargs) const {
    auto it = this->db.templates.find(ExpressionHasher::named_type_hash(link_type));
    if (it != this->db.templates.end()) {
        if (kwargs.toplevel_only) {
            return this->_filter_non_toplevel(it->second);
        }
        return it->second;
    }
    return {};
}

//------------------------------------------------------------------------------
const optional<const string> InMemoryDB::get_atom_type(const string& handle) const {
    auto atom = this->_get_atom(handle);
    if (atom) {
        return atom->named_type;
    }
    return nullopt;
}

//------------------------------------------------------------------------------
const unordered_map<string, int> InMemoryDB::count_atoms() const {
    auto node_count = this->db.node.size();
    auto link_count = this->db.link.size();
    auto atom_count = node_count + link_count;
    return {{"node_count", node_count}, {"link_count", link_count}, {"atom_count", atom_count}};
}

//------------------------------------------------------------------------------
void InMemoryDB::clear_database() { this->db = Database(); }

//------------------------------------------------------------------------------
const shared_ptr<const Node> InMemoryDB::add_node(const Node& node_params) {
    auto node = this->_build_node(node_params);
    this->db.node[node->handle] = node;
    this->_update_index(*node);
    return move(node);
}

//------------------------------------------------------------------------------
const shared_ptr<const Link> InMemoryDB::add_link(const Link& link_params, bool toplevel) {
    auto link = this->_build_link(link_params, toplevel);
    this->db.link[link->handle] = link;
    this->_update_index(*link);
    return move(link);
}

//------------------------------------------------------------------------------
void InMemoryDB::reindex(
    const unordered_map<string, vector<unordered_map<string, any>>>& pattern_index_templates) {
    throw runtime_error("Not implemented");
}

//------------------------------------------------------------------------------
void InMemoryDB::delete_atom(const string& handle) {
    auto it = this->db.node.find(handle);
    if (it != this->db.node.end()) {
        this->db.node.erase(it);
        auto it = this->db.incoming_set.find(handle);
        if (it != this->db.incoming_set.end()) {
            auto handles = move(it->second);
            this->db.incoming_set.erase(it);
            for (const auto& h : handles) {
                this->_delete_link_and_update_index(h);
            }
        }
    } else {
        try {
            this->_delete_link_and_update_index(handle);
        } catch (const AtomDoesNotExist& e) {
            // TODO: log error
            throw AtomDoesNotExist("Nonexistent atom", "handle: " + handle);
        }
    }
}

//------------------------------------------------------------------------------
const string InMemoryDB::create_field_index(const string& atom_type,
                                            const StringList& fields,
                                            const string& named_type,
                                            const optional<const StringList>& composite_type,
                                            FieldIndexType index_type) {
    throw runtime_error("Not implemented");
}

//------------------------------------------------------------------------------
void InMemoryDB::bulk_insert(const vector<shared_ptr<const Atom>>& documents) {
    try {
        for (const auto& document : documents) {
            if (auto node = dynamic_cast<const Node*>(document.get())) {
                this->db.node[document->_id] = make_shared<Node>(*node);
            } else if (auto link = dynamic_cast<const Link*>(document.get())) {
                this->db.link[document->_id] = make_shared<Link>(*link);
            }
            this->_update_index(*document);
        }
    } catch (const exception& e) {
        // TODO: log error
    }
}

//------------------------------------------------------------------------------
const vector<shared_ptr<const Atom>> InMemoryDB::retrieve_all_atoms() const {
    try {
        vector<shared_ptr<const Atom>> atoms;
        atoms.reserve(this->db.node.size() + this->db.link.size());
        for (const auto& [_, node] : this->db.node) {
            atoms.emplace_back(node);
        }
        for (const auto& [_, link] : this->db.link) {
            atoms.emplace_back(link);
        }
        return move(atoms);
    } catch (const exception& e) {
        // TODO: log error
        throw runtime_error("Error retrieving all atoms: " + string(e.what()));
    }
}

//------------------------------------------------------------------------------
void InMemoryDB::commit(const optional<const vector<Atom>>& buffer) {
    throw runtime_error("Not implemented");
}

// PROTECTED OR PRIVATE METHODS ////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
const shared_ptr<const Atom> InMemoryDB::_get_atom(const string& handle) const {
    auto node = this->_get_node(handle);
    if (node) {
        return move(node);
    }
    return this->_get_link(handle);
}

//------------------------------------------------------------------------------
const shared_ptr<const Node> InMemoryDB::_get_node(const string& handle) const {
    auto it = this->db.node.find(handle);
    if (it != this->db.node.end()) {
        return it->second;
    }
    return nullptr;
}

//------------------------------------------------------------------------------
const shared_ptr<const Link> InMemoryDB::_get_link(const string& handle) const {
    auto it = this->db.link.find(handle);
    if (it != this->db.link.end()) {
        return it->second;
    }
    return nullptr;
}

//------------------------------------------------------------------------------
const shared_ptr<const Link> InMemoryDB::_get_and_delete_link(const string& link_handle) {
    auto it = this->db.link.find(link_handle);
    if (it != this->db.link.end()) {
        auto link_document = make_shared<const Link>(*it->second);
        this->db.link.erase(it);
        return move(link_document);
    }
    return nullptr;
}

//------------------------------------------------------------------------------
const StringList InMemoryDB::_build_named_type_hash_template(const StringList& _template) const {
    StringList hash_template;
    hash_template.reserve(_template.size());
    for (const auto& element : _template) {
        hash_template.push_back(move(this->_build_named_type_hash_template(element)));
    }
    return move(hash_template);
}

//------------------------------------------------------------------------------
const string InMemoryDB::_build_named_type_hash_template(const string& _template) const {
    return ExpressionHasher::named_type_hash(_template);
}

//------------------------------------------------------------------------------
void InMemoryDB::_add_outgoing_set(const string& key, const StringList& targets_hash) {
    this->db.outgoing_set[key] = targets_hash;
}

//------------------------------------------------------------------------------
const optional<const StringList> InMemoryDB::_get_and_delete_outgoing_set(const string& handle) {
    auto it = this->db.outgoing_set.find(handle);
    if (it != this->db.outgoing_set.end()) {
        auto handles = move(it->second);
        this->db.outgoing_set.erase(it);
        return move(handles);
    }
    return nullopt;
}

//------------------------------------------------------------------------------
void InMemoryDB::_add_incoming_set(const string& key, const StringList& targets_hash) {
    for (const auto& target_hash : targets_hash) {
        auto it = this->db.incoming_set.find(target_hash);
        if (it == this->db.incoming_set.end()) {
            this->db.incoming_set[target_hash] = StringUnorderedSet({key});
        } else {
            it->second.emplace(key);
        }
    }
}

//------------------------------------------------------------------------------
void InMemoryDB::_delete_incoming_set(const string& link_handle, const StringList& atoms_handles) {
    for (const auto& atom_handle : atoms_handles) {
        auto it = this->db.incoming_set.find(atom_handle);
        if (it != this->db.incoming_set.end()) {
            it->second.erase(link_handle);
        }
    }
}

//------------------------------------------------------------------------------
void InMemoryDB::_add_templates(const string& composite_type_hash,
                                const string& named_type_hash,
                                const string& key) {
    auto it = this->db.templates.find(composite_type_hash);
    if (it != this->db.templates.end()) {
        it->second.emplace(key);
    } else {
        this->db.templates[composite_type_hash] = StringUnorderedSet({key});
    }

    it = this->db.templates.find(named_type_hash);
    if (it != this->db.templates.end()) {
        it->second.emplace(key);
    } else {
        this->db.templates[named_type_hash] = StringUnorderedSet({key});
    }
}

//------------------------------------------------------------------------------
void InMemoryDB::_delete_templates(const Link& link_document) {
    auto it = this->db.templates.find(link_document.composite_type_hash);
    if (it != this->db.templates.end()) {
        it->second.erase(link_document._id);
    }

    it = this->db.templates.find(link_document.named_type_hash);
    if (it != this->db.templates.end()) {
        it->second.erase(link_document._id);
    }
}

//------------------------------------------------------------------------------
void InMemoryDB::_add_patterns(const string& named_type_hash,
                               const string& key,
                               const StringList& targets_hash) {
    auto hash_list = StringList({named_type_hash});
    hash_list.insert(hash_list.end(), targets_hash.begin(), targets_hash.end());
    StringList pattern_keys = build_pattern_keys(hash_list);
    for (const auto& pattern_key : pattern_keys) {
        auto it = this->db.patterns.find(pattern_key);
        if (it == this->db.patterns.end()) {
            this->db.patterns[pattern_key] = StringUnorderedSet({key});
        } else {
            it->second.emplace(key);
        }
    }
}

//------------------------------------------------------------------------------
void InMemoryDB::_delete_patterns(const Link& link_document, const StringList& targets_hash) {
    auto hash_list = StringList({link_document.named_type_hash});
    hash_list.insert(hash_list.end(), targets_hash.begin(), targets_hash.end());
    StringList pattern_keys = build_pattern_keys(hash_list);
    for (const auto& pattern_key : pattern_keys) {
        auto it = this->db.patterns.find(pattern_key);
        if (it != this->db.patterns.end()) {
            it->second.erase(link_document._id);
        }
    }
}

//------------------------------------------------------------------------------
void InMemoryDB::_delete_link_and_update_index(const string& link_handle) {
    auto link_document = this->_get_and_delete_link(link_handle);
    if (link_document) {
        this->_update_index(*link_document, true);
    }
}

//------------------------------------------------------------------------------
const StringUnorderedSet InMemoryDB::_filter_non_toplevel(const StringUnorderedSet& matches) const {
    if (this->db.link.empty()) {
        return matches;
    }
    StringUnorderedSet filtered_matched_targets;
    for (const auto& link_handle : matches) {
        auto it = this->db.link.find(link_handle);
        if (it != this->db.link.end()) {
            if (it->second->is_toplevel) {
                filtered_matched_targets.emplace(link_handle);
            }
        }
    }
    return move(filtered_matched_targets);
}

//------------------------------------------------------------------------------
void InMemoryDB::_delete_atom_index(const Atom& atom) {
    const string& atom_handle = atom._id;
    auto it = this->db.incoming_set.find(atom_handle);
    if (it != this->db.incoming_set.end()) {
        auto handles = move(it->second);
        this->db.incoming_set.erase(it);
        for (const auto& handle : handles) {
            this->_delete_link_and_update_index(handle);
        }
    }

    auto outgoing_atoms = this->_get_and_delete_outgoing_set(atom_handle);
    if (outgoing_atoms.has_value()) {
        this->_delete_incoming_set(atom_handle, *outgoing_atoms);
    }

    if (auto link = dynamic_cast<const Link*>(&atom)) {
        this->_delete_templates(*link);
        this->_delete_patterns(*link, link->targets);
    }
}

//------------------------------------------------------------------------------
void InMemoryDB::_add_atom_index(const Atom& atom) {
    if (auto link = dynamic_cast<const Link*>(&atom)) {
        const string& handle = link->_id;
        this->_add_outgoing_set(handle, link->targets);
        this->_add_incoming_set(handle, link->targets);
        this->_add_templates(link->composite_type_hash, link->named_type_hash, handle);
        this->_add_patterns(link->named_type_hash, handle, link->targets);
    }
}

//------------------------------------------------------------------------------
void InMemoryDB::_update_index(const Atom& atom, bool delete_atom) {
    if (delete_atom) {
        this->_delete_atom_index(atom);
    } else {
        this->_add_atom_index(atom);
    }
}
