#pragma once
#include <set>
#include <string>
#include <memory>

#include "AtomDB.h"

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
    static void reachable_terminal_set_recursive(set<string>& output, shared_ptr<Link> link, bool metta_mapping);

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
    static void reachable_terminal_set(set<string>& output, const string& handle, bool metta_mapping = false);
};

} // namespace atomdb
