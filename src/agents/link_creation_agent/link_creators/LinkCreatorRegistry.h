#pragma once

#include <map>
#include <memory>
#include <mutex>

#include "LinkCreator.h"

using namespace std;

namespace link_creators {

/**
 * Registry for link creation functions used by the link creation agent.
 *
 * In order to register a fitness function, edit LinkCreatorRegistry.cc
 */
class LinkCreatorRegistry {
   public:
    static string REMOTE_FUNCTION;
    static string UNIT_TEST;
    static string CUSTOMIZABLE;
    static string AND_TWO_PREDICATES;

    ~LinkCreatorRegistry() {}
    static shared_ptr<LinkCreator> function(const string& tag);
    static void initialize_statics();

   private:
    LinkCreatorRegistry() {}

    static bool INITIALIZED;
    static map<string, shared_ptr<LinkCreator>> FUNCTION;
};

}  // namespace link_creators
