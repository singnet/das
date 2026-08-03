#include "LinkCreatorRegistry.h"

#include "LinkCreator.h"
#include "Utils.h"

// -------------------------------------------------------------------------------------------------
// Add your header here
//#include "UnitTestFunction.h"
// -------------------------------------------------------------------------------------------------

using namespace link_creators;
using namespace commons;

bool LinkCreatorRegistry::INITIALIZED = false;
string LinkCreatorRegistry::REMOTE_FUNCTION = "remote_link_creation_function";
map<string, shared_ptr<LinkCreator>> LinkCreatorRegistry::FUNCTION;

void LinkCreatorRegistry::initialize_statics() {
    if (INITIALIZED) {
        RAISE_ERROR(
            "LinkCreatorRegistry already initialized. "
            "LinkCreatorRegistry::init() should be called only once.");
    } else {
        INITIALIZED = true;
        // -----------------------------------------------------------------------------------------
        // Add your function here using a unique string key
        // FUNCTION["my_function_tag"] = make_shared<MyFunction>();
        // NOTE: "remote_link_creation_function" is reserved and CAN'T be used here.
        // -----------------------------------------------------------------------------------------
        //FUNCTION["unit_test"] = make_shared<UnitTestFunction>();
    }
}

shared_ptr<LinkCreator> LinkCreatorRegistry::function(const string& tag) {
    if (INITIALIZED) {
        if (tag == REMOTE_FUNCTION) {
            RAISE_ERROR("Invalid use of reserved link creation function tag: " + tag);
        }
        if (FUNCTION.find(tag) != FUNCTION.end()) {
            return FUNCTION[tag];
        } else {
            RAISE_ERROR("Unkown link creation function: " + tag);
        }
    } else {
        RAISE_ERROR(
            "LinkCreatorRegistry isn't initialized. Call "
            "LinkCreatorRegistry::initialize_statics() first.");
    }
    return shared_ptr<LinkCreator>(nullptr);
}
