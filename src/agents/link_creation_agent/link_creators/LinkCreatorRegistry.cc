#include "LinkCreatorRegistry.h"

#include "LinkCreator.h"
#include "Utils.h"

// -------------------------------------------------------------------------------------------------
// ADD your header here
#include "AndTwoPredicates.h"
#include "UnitTestLinkCreator.h"
// -------------------------------------------------------------------------------------------------

using namespace link_creators;
using namespace commons;

bool LinkCreatorRegistry::INITIALIZED = false;
// -----------------------------------------------------------------------------------------
// ADD your function here using a unique string key
// NOTE: "remote_link_creation_function" is reserved and CAN'T be used here.
// -----------------------------------------------------------------------------------------
string LinkCreatorRegistry::REMOTE_FUNCTION = "remote_link_creation_function";
string LinkCreatorRegistry::UNIT_TEST = "unit_test";
string LinkCreatorRegistry::AND_TWO_PREDICATES = "and_two_predicates";

void LinkCreatorRegistry::initialize_statics() {
    STACK_TRACE();
    if (INITIALIZED) {
        RAISE_ERROR(
            "LinkCreatorRegistry already initialized. "
            "LinkCreatorRegistry::init() should be called only once.");
    } else {
        INITIALIZED = true;
    }
}

shared_ptr<LinkCreator> LinkCreatorRegistry::function(const string& tag) {
    STACK_TRACE();
    shared_ptr<LinkCreator> answer = nullptr;
    if (INITIALIZED) {
        if (tag == REMOTE_FUNCTION) {
            RAISE_ERROR("Invalid use of reserved link creation function tag: " + tag);
            // -----------------------------------------------------------------------------------------
            // ADD an "else if" for your function here
        } else if (tag == UNIT_TEST) {
            answer = make_shared<UnitTestLinkCreator>();
        } else if (tag == AND_TWO_PREDICATES) {
            answer = make_shared<AndTwoPredicates>();
            // -----------------------------------------------------------------------------------------
        } else {
            RAISE_ERROR("Unkown link creation function: " + tag);
        }
    } else {
        RAISE_ERROR(
            "LinkCreatorRegistry isn't initialized. Call "
            "LinkCreatorRegistry::initialize_statics() first.");
    }
    return answer;
}
