#include "ProtectedAtomDB.h"

#define LOG_LEVEL INFO_LEVEL
#include <map>
#include <set>

#include "Assignment.h"
#include "AtomDBAPITypes.h"
#include "Logger.h"
#include "Utils.h"

using namespace std;
using namespace atomdb;
using namespace commons;

namespace {

class FilteredHandleSet : public atomdb_api_types::HandleSet {
   public:
    unsigned int size() override { return this->handles.size(); }

    void append(shared_ptr<atomdb_api_types::HandleSet> other) override {
        if (other == nullptr) {
            return;
        }
        auto it = other->get_iterator();
        while (true) {
            char* handle = it->next();
            if (handle == nullptr) {
                break;
            }
            this->add_handle(string(handle),
                             other->get_metta_expressions_by_handle(handle),
                             other->get_assignments_by_handle(handle));
        }
    }

    shared_ptr<atomdb_api_types::HandleSetIterator> get_iterator() override {
        return make_shared<FilteredHandleSetIterator>(this);
    }

    map<string, string> get_metta_expressions_by_handle(const string& handle) override {
        auto it = this->metta_expressions_by_handle.find(handle);
        if (it == this->metta_expressions_by_handle.end()) {
            return {};
        }
        return it->second;
    }

    Assignment get_assignments_by_handle(const string& handle) override {
        auto it = this->assignments_by_handle.find(handle);
        if (it == this->assignments_by_handle.end()) {
            return Assignment();
        }
        return it->second;
    }

    void add_handle(const string& handle,
                    const map<string, string>& metta_expressions,
                    const Assignment& assignment) {
        this->handles.insert(handle);
        this->metta_expressions_by_handle[handle] = metta_expressions;
        this->assignments_by_handle[handle] = assignment;
    }

   private:
    class FilteredHandleSetIterator : public atomdb_api_types::HandleSetIterator {
       public:
        explicit FilteredHandleSetIterator(FilteredHandleSet* handle_set)
            : handle_set(handle_set), it(handle_set->handles.begin()) {}

        char* next() override {
            if (this->it == this->handle_set->handles.end()) {
                return nullptr;
            }
            this->current = *(this->it);
            ++this->it;
            return const_cast<char*>(this->current.c_str());
        }

       private:
        FilteredHandleSet* handle_set;
        set<string>::const_iterator it;
        string current;
    };

    set<string> handles;
    map<string, map<string, string>> metta_expressions_by_handle;
    map<string, Assignment> assignments_by_handle;
};

class FilteredHandleList : public atomdb_api_types::HandleList {
   public:
    void add_handle(const string& handle) { this->handles.push_back(handle); }

    const char* get_handle(unsigned int index) override {
        if (index >= this->handles.size()) {
            return nullptr;
        }
        return this->handles[index].c_str();
    }

    unsigned int size() override { return this->handles.size(); }

   private:
    vector<string> handles;
};

}  // namespace

// --------------------------------------------------------------------------------
// Constructors and destructors

ProtectedAtomDB::ProtectedAtomDB(shared_ptr<AtomDB> backend) : backend(backend) {
    if (this->backend == nullptr) {
        RAISE_ERROR("ProtectedAtomDB requires a non-null backend AtomDB");
    }
    this->manifest = make_shared<AuthorizationManifest>();
    LOG_INFO("ProtectedAtomDB initialized");
}

// --------------------------------------------------------------------------------
// Public methods

shared_ptr<Atom> ProtectedAtomDB::get_atom(const string& handle,
                                           const atomdb_api_types::PublicKey& public_key) {
    if (this->get_protection_mode() == atomdb_api_types::ProtectionMode::FORWARD) {
        RAISE_ERROR(
            "ProtectedAtomDB::get_atom(handle, public_key) is not used in FORWARD mode; "
            "call AtomDBPublicKeyAPI on RemoteAtomDB");
    }
    if (!this->can_read(public_key, handle)) {
        return nullptr;
    }
    return this->backend->get_atom(handle);
}

shared_ptr<Node> ProtectedAtomDB::get_node(const string& handle,
                                           const atomdb_api_types::PublicKey& public_key) {
    return dynamic_pointer_cast<Node>(this->get_atom(handle, public_key));
}

shared_ptr<Link> ProtectedAtomDB::get_link(const string& handle,
                                           const atomdb_api_types::PublicKey& public_key) {
    return dynamic_pointer_cast<Link>(this->get_atom(handle, public_key));
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
                "() is unavailable in protected AtomDBs. Use AtomDBPublicKeyAPI passing a PublicKey.");
}

bool ProtectedAtomDB::ensure_registered(const atomdb_api_types::PublicKey& public_key) {
    if (!public_key.is_single_key()) {
        RAISE_ERROR("ProtectedAtomDB::ensure_registered() multi-key PublicKey is not implemented yet");
    }
    if (public_key.keys.empty()) {
        return false;
    }

    const string& pk = public_key.keys[0];
    if (this->manifest->is_registered(pk)) {
        return true;
    }

    auto docs = this->backend->get_access_permissions(public_key);
    if (docs.empty()) {
        return false;
    }

    for (const auto& doc : docs) {
        if (this->manifest->is_registered(doc->get_access_key())) {
            continue;
        }
        this->manifest->add_document(doc);
    }

    return this->manifest->is_registered(pk);
}

bool ProtectedAtomDB::can_read(const atomdb_api_types::PublicKey& public_key, const string& handle) {
    if (!this->ensure_registered(public_key)) {
        return false;
    }

    HandleDecoder& decoder = *this->backend;
    return this->manifest->is_authorized(
        handle, public_key.keys[0], AuthorizationOperation::READ, decoder);
}

bool ProtectedAtomDB::can_read(const atomdb_api_types::PublicKey& public_key, const atoms::Atom& atom) {
    if (!this->ensure_registered(public_key)) {
        return false;
    }

    HandleDecoder& decoder = *this->backend;
    return this->manifest->is_authorized(
        atom, public_key.keys[0], AuthorizationOperation::READ, decoder);
}

bool ProtectedAtomDB::can_write(const atomdb_api_types::PublicKey& public_key, const atoms::Atom& atom) {
    if (!this->ensure_registered(public_key)) {
        return false;
    }

    HandleDecoder& decoder = *this->backend;
    return this->manifest->is_authorized(
        atom, public_key.keys[0], AuthorizationOperation::WRITE, decoder);
}

bool ProtectedAtomDB::can_write(const atomdb_api_types::PublicKey& public_key, const string& handle) {
    if (!this->ensure_registered(public_key)) {
        return false;
    }

    HandleDecoder& decoder = *this->backend;
    return this->manifest->is_authorized(
        handle, public_key.keys[0], AuthorizationOperation::WRITE, decoder);
}

shared_ptr<atomdb_api_types::HandleSet> ProtectedAtomDB::filter_handle_set(
    shared_ptr<atomdb_api_types::HandleSet> raw, const atomdb_api_types::PublicKey& public_key) {
    auto filtered = make_shared<FilteredHandleSet>();
    if (raw == nullptr) {
        return filtered;
    }

    auto it = raw->get_iterator();
    while (true) {
        char* handle_cstr = it->next();
        if (handle_cstr == nullptr) {
            break;
        }
        string handle(handle_cstr);
        if (this->can_read(public_key, handle)) {
            filtered->add_handle(handle,
                                 raw->get_metta_expressions_by_handle(handle),
                                 raw->get_assignments_by_handle(handle));
        }
    }
    return filtered;
}

shared_ptr<atomdb_api_types::HandleList> ProtectedAtomDB::filter_handle_list(
    shared_ptr<atomdb_api_types::HandleList> raw, const atomdb_api_types::PublicKey& public_key) {
    auto filtered = make_shared<FilteredHandleList>();
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
