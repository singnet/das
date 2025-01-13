#include "database.h"

using namespace std;
using namespace atomdb;

// PUBLIC METHODS //////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
bool AtomDB::node_exists(const string& node_type, const string& node_name) const {
    try {
        this->get_node_handle(node_type, node_name);
        return true;
    } catch (const AtomDoesNotExist& e) {
        return false;
    } catch (const exception& e) {
        string what = e.what();
        if (what.find("Nonexistent atom") != string::npos) return false;
        throw;
    }
}

//------------------------------------------------------------------------------
bool AtomDB::link_exists(const string& link_type, const StringList& target_handles) const {
    try {
        this->get_link_handle(link_type, target_handles);
        return true;
    } catch (const AtomDoesNotExist& e) {
        return false;
    } catch (const exception& e) {
        string what = e.what();
        if (what.find("Nonexistent atom") != string::npos) return false;
        throw;
    }
}

//------------------------------------------------------------------------------
const shared_ptr<const Atom> AtomDB::get_atom(const string& handle, const KwArgs& kwargs) const {
    auto document = _get_atom(handle);
    if (not document) {
        throw AtomDoesNotExist("Nonexistent atom", "handle: " + handle);
    }
    if (not kwargs.no_target_format) {
        return _reformat_document(document, kwargs);
    }
    return move(document);
}

//------------------------------------------------------------------------------
const shared_ptr<const Atom> AtomDB::_reformat_document(const shared_ptr<const Atom>& document,
                                                        const KwArgs& kwargs) const {
    if (const auto& link = dynamic_pointer_cast<const Link>(document)) {
        auto targets_document = kwargs.targets_document;
        auto deep_representation = kwargs.deep_representation;
        if (targets_document or deep_representation) {
            shared_ptr<Link> link_copy = make_shared<Link>(*link);
            link_copy->targets_documents.clear();
            link_copy->targets_documents.reserve(link->targets.size());
            for (const auto& target : link->targets) {
                const auto& atom =
                    deep_representation ? this->get_atom(target, kwargs) : this->get_atom(target);
                if (not atom) continue;
                if (const auto& node = dynamic_pointer_cast<const Node>(atom)) {
                    link_copy->targets_documents.emplace_back(*node);
                } else if (const auto& link = dynamic_pointer_cast<const Link>(atom)) {
                    link_copy->targets_documents.emplace_back(*link);
                }
            }
            return move(link_copy);
        }
    }
    return document;
}

// PROTECTED OR PRIVATE METHODS ////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
shared_ptr<Node> AtomDB::_build_node(const Node& node_params) {
    const auto& node_type = node_params.named_type;
    const auto& node_name = node_params.name;
    if (node_type.empty() or node_name.empty()) {
        // TODO: log error ???
        throw AddNodeException("'type' and 'name' are required.",
                               "node_params: " + node_params.to_string());
    }
    string handle = this->build_node_handle(node_type, node_name);
    string composite_type_hash = ExpressionHasher::named_type_hash(node_type);
    auto node = make_shared<Node>(handle,                        // id
                                  handle,                        // handle
                                  composite_type_hash,           // composite_type_hash
                                  node_type,                     // named_type
                                  node_name,                     // name
                                  node_params.custom_attributes  // custom_attributes
    );
    node->validate();
    return move(node);
}

//------------------------------------------------------------------------------
shared_ptr<Link> AtomDB::_build_link(const Link& link_params, bool is_toplevel) {
    const auto& link_type = link_params.named_type;
    const auto& targets = link_params.targets_documents;
    if (link_type.empty() or targets.empty()) {
        // TODO: log error ???
        throw AddLinkException("'type' and 'targets' are required.",
                               "link_params: " + link_params.to_string() +
                                   ", is_toplevel: " + (is_toplevel ? "true" : "false"));
    }
    string link_type_hash = ExpressionHasher::named_type_hash(link_type);
    StringList target_handles = {};
    ListOfAny composite_type_list = {link_type_hash};
    StringList composite_type_elements = {link_type_hash};
    string atom_hash;
    string atom_handle;
    for (const auto& target : targets) {
        if (auto node_params = std::get_if<Node>(&target)) {
            auto node = this->add_node(*node_params);
            atom_handle = node->_id;
            atom_hash = node->composite_type_hash;
            composite_type_list.emplace_back(atom_hash);
        } else if (auto link_params = std::get_if<Link>(&target)) {
            auto link = this->add_link(*link_params, false);
            atom_handle = link->_id;
            atom_hash = link->composite_type_hash;
            composite_type_list.emplace_back(link->composite_type);
        } else {
            throw invalid_argument("Invalid target type. Must be Node or Link.");
        }
        composite_type_elements.emplace_back(atom_hash);
        target_handles.emplace_back(atom_handle);
    }

    string handle = ExpressionHasher::expression_hash(link_type_hash, target_handles);
    string composite_type_hash = ExpressionHasher::composite_hash(composite_type_elements);

    auto link = make_shared<Link>(handle,                         // id
                                  handle,                         // handle
                                  composite_type_hash,            // composite_type_hash
                                  link_type,                      // named_type
                                  composite_type_list,            // composite_type
                                  link_type_hash,                 // named_type_hash
                                  target_handles,                 // targets
                                  is_toplevel,                    // is_toplevel
                                  link_params.custom_attributes,  // custom_attributes
                                  Link::TargetsDocuments{}        // targets_documents
    );
    link->validate();
    return move(link);
}
