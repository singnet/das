#define LOG_LEVEL DEBUG_LEVEL
#include "AndTwoPredicates.h"
#include "tags.h"

using namespace link_creators;

string AndTwoPredicates::LOGICAL_AND_HANDLE = Hasher::node_handle(SYMBOL, LOGICAL_AND_TAG);
string AndTwoPredicates::EVALUATION_HANDLE = Hasher::node_handle(SYMBOL, EVALUATION_TAG);

// -------------------------------------------------------------------------------------------------
// Public methods

AndTwoPredicates::AndTwoPredicates() {
}

AndTwoPredicates::~AndTwoPredicates() {
}

LinkCreationStats AndTwoPredicates::create(shared_ptr<QueryAnswer> query_answer) {
    STACK_TRACE();
    string concept_ = query_answer->get(CONCEPT);
    string predicates[2];
    predicates[0] = query_answer->get(PREDICATE1);
    predicates[1] = query_answer->get(PREDICATE2);
    if (predicates[1] < predicates[0]) {
        string aux = predicates[0];
        predicates[0] = predicates[1];
        predicates[1] = aux;
    }
    string key = predicates[0] + " " + predicates[1];

    LinkCreationStats stats = LinkCreationStats(false, 0, 0);
    if (predicates[0] != predicates[1]) {
        if (! visited(key)) {
            visit(key);
            stats.visited = true;
            set<string> mentioned_predicates0, mentioned_predicates1;
            extract_mentioned_predicates(mentioned_predicates0, predicates[0]);
            extract_mentioned_predicates(mentioned_predicates1, predicates[1]);
            if (! Utils::intersects(mentioned_predicates0, mentioned_predicates1)) {
                vector<string> targets = {LOGICAL_AND_HANDLE, predicates[0], predicates[1]};
                if (add_or_update_link(targets, 1.0)) {
                    stats.created++;
                    double strength = 1;
                    for (string& h : query_answer->get_handles_vector()) {
                        strength *= get_strength(h);
                    }
                    if (strength >= strength_threshold()) {
                        stats.created++;
                        string new_predicate_handle = Hasher::link_handle(EXPRESSION, targets);
                        add_or_update_link({EVALUATION_HANDLE, new_predicate_handle, concept_}, strength);
                    } else {
                        stats.updated++;
                    }
                } else {
                    LOG_DEBUG("(" + Utils::join(vector<string>(mentioned_predicates0.begin(), mentioned_predicates0.end()), '-') + ", " + Utils::join(vector<string>(mentioned_predicates1.begin(), mentioned_predicates1.end()), '-') + ") " + "Skipping link building because composite predicate already exists.");
                }
            } else {
                LOG_DEBUG("(" + Utils::join(vector<string>(mentioned_predicates0.begin(), mentioned_predicates0.end()), '-') + ", " + Utils::join(vector<string>(mentioned_predicates1.begin(), mentioned_predicates1.end()), '-') + ") " + "Skipping link building because predicates intersect.");
            }
        } else {
            LOG_DEBUG("(" + predicates[0] + ", " + predicates[1] + ") " + "Skipping link building because targets have already been visited this cycle: " + key);
        }
    } else {
        LOG_DEBUG("(" + predicates[0] + ", " + predicates[1] + ") " + "Skipping link building because predicates are the same.");
    }
    return stats;
}

void AndTwoPredicates::extract_mentioned_predicates(set<string>& mentioned, const string& handle) {
    STACK_TRACE();
    shared_ptr<Node> node;
    shared_ptr<Link> link = atomdb()->get_link(handle);
    if (link != nullptr) {
        for (string& target_handle : link->targets) {
            if ((node = atomdb()->get_node(target_handle)) != nullptr) {
                if ((node->name != PREDICATE_TAG) && (node->name != LOGICAL_AND_TAG)) {
                    mentioned.insert(node->name);
                }
            } else {
                extract_mentioned_predicates(mentioned, target_handle);
            }
        }
    }
}
