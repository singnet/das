#pragma once
#include <map>
#include <memory>
#include <set>
#include <string>

#include "AtomDB.h"
#include "HandleDecoder.h"
#include "KeySensitiveAtomDB.h"
#include "Keychain.h"

using namespace std;

namespace atomdb {

/**
 *
 */
class AtomDBUtils : public HandleDecoder {
   public:
    AtomDBUtils() {}
    ~AtomDBUtils() {}

   public:
    /**
     * The reachable set of a given Link contains any Node in its target list plus any Node
     * reachable through the recursive application of this method to the Links in its
     * targets list. The reachable set of a Node is the Node itself.
     *
     * @param output A std::set where output is supposed to be placed.
     * @param handle The handle of the starting link. If a Node handle is passed
     * instead, the output will be this single handle.
     * is being used, the first element in a expression is not considered to be put in the output.
     */
    static void reachable_terminal_set(set<string>& output,
                                       const string& handle,
                                       bool name_flag = false,
                                       shared_ptr<Keychain> keychain = nullptr);

    /**
     * Counts the number of nodes/links/atoms which can be reachable by the passed handle.
     *
     * @param handle The starting point for the count.
     * @param node_count Output for the number of nodes.
     * @param link_count Output for the number of links.
     * @param atom_count Output for the number of atoms.
     * @param keychain Optional keychain.
     * @param visited A record of atom which has already been visited (thus, are disregarded).
     */
    static void count_reachable_atoms(const string& handle,
                                      size_t& node_count,
                                      size_t& link_count,
                                      size_t& atom_count,
                                      shared_ptr<Keychain> keychain,
                                      set<string>& visited);

    /**
     * Build a metta expression out of an atom handle. Optionally, a keychain can be passed
     * to be used when the AtomDB is protected.
     *
     * @param handle The handle whose metta expression we want to build.
     * @param keychain Keychain with public keys to be passed when the AtomDB is protected.
     */
    static string handle_to_metta(const string& handle, shared_ptr<Keychain> keychain = nullptr);

    /**
     * Build a metta expression out of an atom handle. All the internal sub-expressions
     * (as well the toplevel one) are inserted in the passed map handle -> metta expression.
     *
     * @param handle The handle whose metta expression we want to build.
     * @param keychain Keychain with public keys to be passed when the AtomDB is protected.
     */
    static string handle_to_metta(const string& handle,
                                  map<string, string>& mapping,
                                  shared_ptr<Keychain> keychain = nullptr);

    /**
     * Return the Atom, given its handle. nullptr is returned if Atom doesn't exist.
     *
     * @param handle Handle of the atom to be returned.
     * @param keychain Optional keychain to be passed to KeySentitive AtomDB.
     * @return the Atom, given its handle. nullptr is returned if Atom doesn't exist.
     */
    static shared_ptr<Atom> get_atom(const string& handle, shared_ptr<Keychain> keychain = nullptr);

    /**
     * Return the Node, given its handle. nullptr is returned if Node doesn't exist.
     *
     * @param handle Handle of the node to be returned.
     * @param keychain Optional keychain to be passed to KeySentitive AtomDB.
     * @return the Node, given its handle. nullptr is returned if Node doesn't exist.
     */
    static shared_ptr<Node> get_node(const string& handle, shared_ptr<Keychain> keychain = nullptr);

    /**
     * Return the Link, given its handle. nullptr is returned if Link doesn't exist.
     *
     * @param handle Handle of the link to be returned.
     * @param keychain Optional keychain to be passed to KeySentitive AtomDB.
     * @return the Link, given its handle. nullptr is returned if Link doesn't exist.
     */
    static shared_ptr<Link> get_link(const string& handle, shared_ptr<Keychain> keychain = nullptr);

    /**
     * Return a node's name or "" if it doesn't exist.
     *
     * @param handle Handle of the node whose name is to be returned.
     * @return a node's name or "" if it doesn't exist.
     */
    static string get_node_name(const string& handle, shared_ptr<Keychain> keychain = nullptr);

    /**
     * Return a link's targets.
     *
     * @param handle Handle of the link whose targets are to be returned.
     * @return a link's targets.
     */
    static vector<string> get_link_targets(const string& handle,
                                           shared_ptr<Keychain> keychain = nullptr);

    /**
     * Return an atom's strength or 0.0 if it doesn't exist.
     *
     * @param handle Handle of the atom whose strength is to be returned.
     * @return an atom's strength or raises if it doesn't exist.
     */
    static double get_strength(const string& handle,
                               shared_ptr<Keychain> keychain = nullptr,
                               const string& strength_tag = "strength");

    // HandleDecoder interface

    shared_ptr<Atom> decode_atom(const string& handle, shared_ptr<Keychain> keychain = nullptr) override;
    string decode_node_name(const string& handle, shared_ptr<Keychain> keychain = nullptr) override;
    vector<string> decode_link_targets(const string& handle,
                                       shared_ptr<Keychain> keychain = nullptr) override;

   private:
    static string handle_to_metta_recursive(const string& handle,
                                            map<string, string>& mapping,
                                            bool populate_mapping,
                                            shared_ptr<Keychain> keychain);
};

}  // namespace atomdb
