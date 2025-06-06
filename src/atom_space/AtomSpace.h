#pragma once

#include <memory>

#include "AtomDBAPITypes.h"
#include "AtomDBSingleton.h"
#include "AtomSpaceTypes.h"
#include "PatternMatchingQueryProxy.h"
#include "ServiceBusSingleton.h"

#define IGNORE_ANSWER_COUNT 0

using namespace std;
using namespace atomdb;
using namespace atomdb_api_types;
using namespace commons;
using namespace service_bus;
using namespace query_engine;

namespace atomspace {

// -------------------------------------------------------------------------------------------------
/**
 * @brief AtomSpace class for managing atoms, nodes, and links.
 *
 * This class provides methods to retrieve, add, and commit atoms, nodes, and links,
 * as well as to perform pattern matching queries via a service bus.
 */
class AtomSpace {
   public:
    /**
     * @brief Scope for atom retrieval and commit operations.
     * LOCAL_ONLY: Only search or commit locally.
     * REMOTE_ONLY: Only search or commit remotely.
     * LOCAL_AND_REMOTE: Try local first, then remote if not found.
     */
    enum Scope { LOCAL_ONLY, REMOTE_ONLY, LOCAL_AND_REMOTE };

    /**
     * @brief Construct an AtomSpace with a given ServiceBus.
     */
    AtomSpace();

    /**
     * @brief Retrieve an atom by handle.
     *
     * LOCAL_ONLY and REMOTE_ONLY look for the atom in db or by issuing a service bus command
     * accordingly. For LOCAL_AND_REMOTE, look for the atom in the db. If present, return it.
     * Otherwise, issue a command in the bus.
     *
     * @param handle The atom handle.
     * @param scope Where to search for the atom (default: LOCAL_AND_REMOTE).
     * @return Pointer to the Atom, or nullptr if not found.
     */
    const Atom* get_atom(const char* handle, Scope scope = LOCAL_AND_REMOTE);

    /**
     * @brief Retrieve a node by type and name.
     *
     * LOCAL_ONLY and REMOTE_ONLY look for the node in db or by issuing a service bus command
     * accordingly. For LOCAL_AND_REMOTE, look for the node in the db. If present, return it.
     * Otherwise, issue a command in the bus.
     *
     * @param type Node type.
     * @param name Node name.
     * @param scope Where to search for the node (default: LOCAL_AND_REMOTE).
     * @return Pointer to the Node, or nullptr if not found.
     */
    const Node* get_node(const string& type, const string& name, Scope scope = LOCAL_AND_REMOTE);

    /**
     * @brief Retrieve a link by type and targets.
     *
     * LOCAL_ONLY and REMOTE_ONLY look for the link in db or by issuing a service bus command
     * accordingly. For LOCAL_AND_REMOTE, look for the link in the db. If present, return it.
     * Otherwise, issue a command in the bus.
     *
     * @param type Link type.
     * @param targets List of target atoms.
     * @param scope Where to search for the link (default: LOCAL_AND_REMOTE).
     * @return Pointer to the Link, or nullptr if not found.
     */
    const Link* get_link(const string& type,
                         const vector<const Atom*>& targets,
                         Scope scope = LOCAL_AND_REMOTE);

    /**
     * @brief Issue a pattern matching query via the service bus.
     *
     * @param query The query pattern.
     * @param answers_count Maximum number of answers to return (0 = all).
     * @param context Optional context string.
     * @param unique_assignment Whether to enforce unique variable assignments.
     * @param update_attention_broker Whether to update the attention broker.
     * @param count_only If true, only count the matches.
     * @return Shared pointer to the PatternMatchingQueryProxy.
     */
    shared_ptr<PatternMatchingQueryProxy> pattern_matching_query(
        const vector<string>& query,
        size_t answers_count = IGNORE_ANSWER_COUNT,
        const string& context = "",
        bool unique_assignment = true,
        bool update_attention_broker = false,
        bool count_only = false);

    /**
     * @brief Count the number of pattern matches via the service bus.
     *
     * @param query The query pattern.
     * @param context Optional context string.
     * @param unique_assignment Whether to enforce unique variable assignments.
     * @param update_attention_broker Whether to update the attention broker.
     * @return Number of matches.
     */
    size_t pattern_matching_count(const vector<string>& query,
                                  const string& context = "",
                                  bool unique_assignment = true,
                                  bool update_attention_broker = false);

    /**
     * @brief Fetch Atoms of a pattern matching results from the remote database and store them
     * locally.
     *
     * @param query The query pattern.
     * @param answers_count Maximum number of answers to fetch (0 = all).
     */
    void pattern_matching_fetch(const vector<string>& query, size_t answers_count = IGNORE_ANSWER_COUNT);

    /**
     * @brief Add a node to the AtomSpace.
     *
     * Atoms are instantiated inside add_node(). Atom handle is computed (and NOT stored in the Atom
     * object). Atom pointer is stored directly in the HandleTrie using the computed handle as key.
     *
     * @param type Node type.
     * @param name Node name.
     * @return The handle of the new node (caller is responsible for freeing).
     */
    char* add_node(const string& type, const string& name);

    /**
     * @brief Add a link to the AtomSpace.
     *
     * Atoms are instantiated inside add_link(). Atom handle is computed (and NOT stored in the Atom
     * object). Atom pointer is stored directly in the HandleTrie using the computed handle as key.
     *
     * @param type Link type.
     * @param targets List of target atoms.
     * @return The handle of the new link (caller is responsible for freeing).
     */
    char* add_link(const string& type, const vector<const Atom*>& targets);

    /**
     * @brief Commit changes to the AtomSpace.
     *
     * If LOCAL scope is selected (LOCAL_ONLY or LOCAL_AND_REMOTE), commit ALL atoms in local db by
     * sending them to the server (by issuing a command in the service bus). Atoms should be kept in
     * the local db. The command should wait this sync ends before going to the next step. Once local
     * changes are committed, issue a DB COMMIT command in the service bus.
     *
     * @param scope Where to commit changes (default: LOCAL_AND_REMOTE).
     */
    void commit_changes(Scope scope = LOCAL_AND_REMOTE);

   private:
    shared_ptr<ServiceBus> bus;
    unique_ptr<HandleTrie> handle_trie;
    shared_ptr<AtomDB> db;

    /**
     * @brief Create an Atom from a document.
     *
     * @param document The AtomDocument to create the Atom from.
     * @return A pointer to the created Atom, or NULL if the document is invalid.
     * @throws std::runtime_error if the document is invalid (does not contain "targets" or "name").
     * @note The caller is responsible for deleting the returned Atom.
     */
    const Atom* atom_from_document(const shared_ptr<AtomDocument>& document);
};

}  // namespace atomspace
