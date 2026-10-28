#pragma once
#include <map>
#include <memory>
#include <set>
#include <string>

#include "AtomDB.h"
#include "KeySensitiveAtomDB.h"
#include "Keychain.h"

using namespace std;

namespace atomdb {

/**
 *
 */
class AtomDBUtils {
   public:
    AtomDBUtils();
    ~AtomDBUtils();

   private:
    static void reachable_terminal_set_recursive(set<string>& output,
                                                 shared_ptr<Link> link,
                                                 bool metta_mapping,
                                                 bool name_flag);

    static string handle_to_metta_recursive(const string& handle,
                                            map<string, string>& mapping,
                                            bool populate_mapping,
                                            shared_ptr<Keychain> keychain,
                                            shared_ptr<AtomDB> atomdb,
                                            shared_ptr<KeySensitiveAtomDB> protected_atomdb);

    static void count_reachable_atoms_recursive(shared_ptr<Link> link,
                                                size_t& node_count,
                                                size_t& link_count,
                                                size_t& atom_count,
                                                shared_ptr<Keychain> keychain,
                                                set<string>& visited);

   public:
    /**
     * The reachable set of a given Link contains any Node in its target list plus any Node
     * reachable through the recursive application of this method to the Links in its
     * targets list. The reachable set of a Node is the Node itself.
     *
     * @param output A std::set where output is supposed to be placed.
     * @param handle The handle of the starting link. If a Node handle is passed
     * instead, the output will be this single handle.
     * @param metta_mapping Optional flag to indicate the use of MeTTa mapping. When MeTTa mapping
     * is being used, the first element in a expression is not considered to be put in the output.
     */
    static void reachable_terminal_set(set<string>& output,
                                       const string& handle,
                                       bool metta_mapping = false,
                                       bool name_flag = false);
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
     * @param metta_mapping A handle -> metta expression map to be used to store mappings.
     * @param keychain Keychain with public keys to be passed when the AtomDB is protected.
     */
    static string handle_to_metta(const string& handle,
                                  map<string, string>& mapping,
                                  shared_ptr<Keychain> keychain = nullptr);

    /**
     * Return a node's name or "" if it doesn't exist.
     *
     * @param handle Handle of the node whose name is to be returned.
     * @return a node's name or "" if it doesn't exist.
     */
    static string get_node_name(const string& handle);

    /**
     * Return an atom's strength or raises if it doesn't exist.
     *
     * @param handle Handle of the atom whose strength is to be returned.
     * @return an atom's strength or raises if it doesn't exist.
     */
    static double get_strength(const string& handle, shared_ptr<Keychain> keychain = nullptr, const string& strength_tag = "strength");
};

}  // namespace atomdb
