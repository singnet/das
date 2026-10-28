#include "AndTwoPredicates.h"
#include "AtomDBUtils.h"

#include "tags.h"

using namespace link_creators;

string AndTwoPredicates::LOGICAL_AND_HANDLE = Hasher::node_handle(SYMBOL, LOGICAL_AND_TAG);
string AndTwoPredicates::EVALUATION_HANDLE = Hasher::node_handle(SYMBOL, EVALUATION_TAG);

// -------------------------------------------------------------------------------------------------
// Public methods

AndTwoPredicates::AndTwoPredicates() {}

AndTwoPredicates::~AndTwoPredicates() {}

LinkCreationStats AndTwoPredicates::create(shared_ptr<QueryAnswer> query_answer) {
    STACK_TRACE();
    LinkCreationStats stats;
    double strength = 1;
    for (string& h : query_answer->get_handles_vector()) {
        strength *= AtomDBUtils::get_strength(h);
    }
    if (!Utils::epsilon_equals(strength, 1.0)) {
        LOG_DEBUG("AndTwoPredicates discarding low strength (" + std::to_string(strength) + ") link. QueryAnswer: " + query_answer->to_string(true));
        return stats;
    }
    string concept_ = query_answer->get(CONCEPT);
    string predicates[2];
    predicates[0] = query_answer->get(PREDICATE1);
    predicates[1] = query_answer->get(PREDICATE2);
    if (predicates[1] < predicates[0]) {
        string aux = predicates[0];
        predicates[0] = predicates[1];
        predicates[1] = aux;
    }
    string key = predicates[0] + " " + predicates[1] + " " + concept_;

    if (predicates[0] != predicates[1]) {
        if (!visited(key)) {
            visit(key);
            stats.visited = true;
            AddLinkStatus add_status;
            set<string> mentioned_predicates0, mentioned_predicates1;
            AtomDBUtils::reachable_terminal_set(mentioned_predicates0, predicates[0], true, true);
            AtomDBUtils::reachable_terminal_set(mentioned_predicates1, predicates[1], true, true);
            if (!Utils::intersects(mentioned_predicates0, mentioned_predicates1)) {
                vector<string> targets = {LOGICAL_AND_HANDLE, predicates[0], predicates[1]};
                add_status = add_or_update_link(targets, 1.0);
                if (add_status == CREATED) {
                    stats.created++;
                } else if (add_status == UPDATED) {
                    stats.updated++;
                }
                string new_predicate_handle = Hasher::link_handle(EXPRESSION, targets);
                AddLinkStatus add_status =
                    add_or_update_link({EVALUATION_HANDLE, new_predicate_handle, concept_}, strength);
                if (add_status == CREATED) {
                    stats.created++;
                } else if (add_status == UPDATED) {
                    stats.updated++;
                }
            } else {
                LOG_DEBUG("(" +
                          Utils::join(
                              vector<string>(mentioned_predicates0.begin(), mentioned_predicates0.end()),
                              '-') +
                          ", " +
                          Utils::join(
                              vector<string>(mentioned_predicates1.begin(), mentioned_predicates1.end()),
                              '-') +
                          ") " + "Skipping link building because predicates intersect.");
            }
        } else {
            LOG_DEBUG("Skipping link building because targets have already been visited this cycle: " +
                      key);
        }
    } else {
        LOG_DEBUG("(" + predicates[0] + ", " + predicates[1] + ") " +
                  "Skipping link building because predicates are the same.");
    }
    return stats;
}
