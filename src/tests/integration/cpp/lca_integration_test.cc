#include "AndTwoPredicates.h"
#include "AtomDBSingleton.h"
#include "AtomDBUtils.h"
#include "CustomizableLinkCreator.h"
#include "JsonConfigParser.h"
#include "LinkCreationProxy.h"
#include "LinkCreatorRegistry.h"
#include "Logger.h"
#include "PatternMatchingQueryProxy.h"
#include "QueryAnswer.h"
#include "ServiceBusSingleton.h"
#include "SystemParametersSingleton.h"
#include "Utils.h"
#include "tags.h"

using namespace std;
using namespace atomdb;
using namespace commons;
using namespace service_bus;
using namespace link_creators;
using namespace link_creation_agent;

static void insert_type_symbols() {
    STACK_TRACE();
    vector<string> to_insert = {EQUIVALENCE_TAG,
                                IMPLICATION_TAG,
                                LOGICAL_AND_TAG,
                                "FunctionalTest0",
                                "FunctionalTest1",
                                "FunctionalTest2",
                                "FunctionalTest3"};
    Node* node;
    for (string node_name : to_insert) {
        node = new Node(SYMBOL, node_name);
        AtomDBSingleton::get_instance()->add_node(node);
        delete (node);
    }
}

static string start_test_case(const string& test_case) {
    LOG_INFO("--------------------------------------------------------------------------------");
    LOG_INFO("START " + test_case);
    LOG_INFO("Atom count: " + to_string(AtomDBSingleton::get_instance()->atom_count()));
    return test_case;
}

static void finish_test_case(const string& test_case, bool success) {
    LOG_INFO("Atom count: " + to_string(AtomDBSingleton::get_instance()->atom_count()));
    if (success) {
        LOG_INFO("OK - " + test_case);
    } else {
        LOG_INFO("FAILED - " + test_case);
    }
}

static bool assert_equal(unsigned int v1, unsigned int v2, const string& tag) {
    if (v1 == v2) {
        LOG_DEBUG("ASSERT PASSED - " + tag + ": " + to_string(v1) + " == " + to_string(v2));
        return true;
    } else {
        LOG_INFO("ASSERT FAILED - " + tag + ": " + to_string(v1) + " != " + to_string(v2));
        return false;
    }
}

static bool assert_true(bool v, const string& tag) {
    if (v) {
        LOG_DEBUG("ASSERT PASSED - " + tag);
    } else {
        LOG_INFO("ASSERT FAILED - " + tag);
    }
    return v;
}

static void timeout_after_minutes(unsigned int minutes) {
    std::thread t(
        [](unsigned int minutes) {
            Utils::sleep(minutes * 60000);
            LOG_INFO("================================================================================");
            LOG_INFO("TIMEOUT after " + to_string(minutes) + " minute" +
                     string(minutes != 1 ? "s" : ""));
            exit(1);
        },
        minutes);
    t.detach();
}

shared_ptr<LinkCreationProxy> make_proxy(
    const vector<string>& query_tokens,
    const string& link_creator,
    BaseProxy::ORCHESTRATION_SCHEMA_TYPE orchestration = BaseProxy::NONE) {
    auto proxy = make_shared<LinkCreationProxy>(query_tokens, "", link_creator, orchestration);
    proxy->parameters[LinkCreationProxy::MAX_SUCCESSFUL_CREATION_PER_ROUND] = (unsigned int) 0;
    proxy->parameters[LinkCreationProxy::MAX_UNPRODUCTIVE_VISITS_PER_ROUND] = (unsigned int) 0;
    proxy->parameters[LinkCreationProxy::MAX_VISIT_ATTEMPTS_PER_ROUND] = (unsigned int) 0;
    proxy->parameters[LinkCreationProxy::MAX_ROUNDS] = (unsigned int) 1;
    proxy->parameters[LinkCreationProxy::LINK_CREATION_STRENGTH_THRESHOLD] = (double) 0;
    proxy->parameters[LinkCreationProxy::LINK_CREATION_LOG_FILE_NAME] = (string) "";
    proxy->parameters[LinkCreationProxy::LOG_NEW_LINKS] = (bool) false;
    proxy->parameters[PatternMatchingQueryProxy::DISREGARD_IMPORTANCE_FLAG] = (bool) false;
    proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = (bool) false;
    proxy->parameters[PatternMatchingQueryProxy::UNIQUE_VALUE_FLAG] = (bool) true;
    proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = (bool) true;
    return proxy;
}

static bool test_and_two_predicates() {
    STACK_TRACE();

    string test_case = start_test_case("test_and_two_predicates()");
    bool success = true;
    // clang-format off
    vector<string> query_tokens = {
        AND_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION_TAG,
                VARIABLE, PREDICATE1,
                VARIABLE, CONCEPT,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION_TAG,
                VARIABLE, PREDICATE2,
                VARIABLE, CONCEPT
    };
    // clang-format on
    auto proxy = make_proxy(query_tokens, LinkCreatorRegistry::AND_TWO_PREDICATES);
    ServiceBusSingleton::get_instance()->issue_bus_command(proxy);

    while (true) {
        if (proxy->finished()) {
            break;
        }
        shared_ptr<QueryAnswer> answer = proxy->pop();
        if (answer == nullptr) {
            Utils::sleep();
        }
    }
    success &= assert_equal(proxy->get_count(), 10530, "link creation count");
    AtomDBSingleton::get_instance()->delete_atoms(proxy->get_built_atoms());

    finish_test_case(test_case, success);
    return success;
}

static double get_strength(const string& handle) {
    STACK_TRACE();
    double answer = 1.0;
    auto atom = AtomDBSingleton::get_instance()->get_atom(handle);
    if (atom == nullptr) {
        RAISE_ERROR("Atom does not exist: " + handle);
    } else {
        answer = atom->custom_attributes.get_or<double>(STRENGTH_TAG, 1.0);
    }
    return answer;
}

static double compute_expected_strength(const string& handle1,
                                        const string& handle2,
                                        CustomizableLinkCreator::StrengthComposition composition) {
    auto db = AtomDBSingleton::get_instance();
    auto concept_link1 = db->get_link(handle1);
    auto concept_link2 = db->get_link(handle2);
    if ((concept_link1 != nullptr) && (concept_link2 != nullptr)) {
        auto terminal1 = db->get_node(concept_link1->targets[1]);
        auto terminal2 = db->get_node(concept_link2->targets[1]);
        if ((terminal1 != nullptr) && (terminal2 != nullptr)) {
            string s1 = terminal1->name.substr(1, terminal1->name.size() - 2);
            string s2 = terminal2->name.substr(1, terminal2->name.size() - 2);
            LOG_DEBUG("Computing expected strength for: \"" + s1 + "\" and \"" + s2 + "\"");
            vector<string> v;
            v = Utils::split(s1);
            set<string> set1(v.begin(), v.end());
            set1.insert("begins_with_" + v[0]);
            set1.insert("ends_with_" + v[v.size() - 1]);
            set1.insert("sort");
            v.clear();
            v = Utils::split(s2);
            set<string> set2(v.begin(), v.end());
            set2.insert("begins_with_" + v[0]);
            set2.insert("ends_with_" + v[v.size() - 1]);
            set2.insert("sort");
            std::set<string> _intersection, _union;
            std::set_intersection(set1.begin(),
                                  set1.end(),
                                  set2.begin(),
                                  set2.end(),
                                  std::inserter(_intersection, _intersection.begin()));
            std::set_union(set1.begin(),
                           set1.end(),
                           set2.begin(),
                           set2.end(),
                           std::inserter(_union, _union.begin()));
            LOG_DEBUG("Counts: " << set1.size() << " " << set2.size() << " " << _intersection.size()
                                 << " " << _union.size());

            switch (composition) {
                case CustomizableLinkCreator::INTERSECTION_OVER_UNION:
                    return (_intersection.size() == 0) ? 0
                                                       : ((double) _intersection.size() / _union.size());
                case CustomizableLinkCreator::INTERSECTION_OVER_A:
                    return (set1.size() == 0) ? 0 : ((double) _intersection.size() / set1.size());
                case CustomizableLinkCreator::INTERSECTION_OVER_B:
                    return (set2.size() == 0) ? 0 : ((double) _intersection.size() / set2.size());
                default:
                    RAISE_ERROR("Invalid composition: " + std::to_string((unsigned int) composition));
            }
        }
    }
    return 0;
}

static bool test_customizable() {
    string test_case = start_test_case("test_customizable()");
    bool success = true;
    vector<string> tokens;

    // clang-format off
    vector<string> query_tokens1 = {
        AND_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION_TAG,
                LINK, EXPRESSION, "2",
                    NODE, SYMBOL, PREDICATE_TAG,
                    NODE, SYMBOL, "\"contains_bbb\"",
                VARIABLE, "v1",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION_TAG,
                LINK, EXPRESSION, "2",
                    NODE, SYMBOL, PREDICATE_TAG,
                    NODE, SYMBOL, "\"contains_ccc\"",
                VARIABLE, "v2",
    };
    // clang-format on
    CustomizableLinkCreator link_creator1;
    link_creator1.add_link_specification({QueryAnswerElement("v1"), QueryAnswerElement("v2")},
                                         {},
                                         "FunctionalTest1",
                                         CustomizableLinkCreator::PRODUCT);
    link_creator1.add_link_specification({QueryAnswerElement("v2"), QueryAnswerElement("v1")},
                                         {},
                                         "FunctionalTest2",
                                         CustomizableLinkCreator::PRODUCT);
    tokens.clear();
    link_creator1.tokenize(tokens);
    auto proxy1 = make_proxy(query_tokens1, LinkCreatorRegistry::CUSTOMIZABLE);
    proxy1->parameters[LinkCreationProxy::LINK_CREATOR_EXTRA_PARAMETERS] =
        (string) Utils::join(tokens, ',');
    proxy1->parameters[LinkCreationProxy::MAX_SUCCESSFUL_CREATION_PER_ROUND] = (unsigned int) 200;

    ServiceBusSingleton::get_instance()->issue_bus_command(proxy1);

    while (true) {
        if (proxy1->finished()) {
            break;
        }
        shared_ptr<QueryAnswer> answer = proxy1->pop();
        if (answer == nullptr) {
            Utils::sleep();
        }
    }
    success &= assert_equal(proxy1->get_count(), 200, "link creation count");

    // clang-format off
    vector<string> query_tokens2 = {
        AND_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, "FunctionalTest1",
                VARIABLE, "v1",
                VARIABLE, "v2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, "FunctionalTest2",
                VARIABLE, "v2",
                VARIABLE, "v1",
    };
    // clang-format on
    CustomizableLinkCreator link_creator2;
    link_creator2.add_link_specification({QueryAnswerElement("v1"), QueryAnswerElement("v2")},
                                         {},
                                         "FunctionalTest3",
                                         CustomizableLinkCreator::PRODUCT);
    tokens.clear();
    link_creator2.tokenize(tokens);
    auto proxy2 = make_proxy(query_tokens2, LinkCreatorRegistry::CUSTOMIZABLE);
    proxy2->parameters[LinkCreationProxy::LINK_CREATOR_EXTRA_PARAMETERS] =
        (string) Utils::join(tokens, ',');

    ServiceBusSingleton::get_instance()->issue_bus_command(proxy2);

    while (true) {
        if (proxy2->finished()) {
            break;
        }
        shared_ptr<QueryAnswer> answer = proxy2->pop();
        if (answer == nullptr) {
            Utils::sleep();
        }
    }
    success &= assert_equal(proxy2->get_count(), 200, "link creation count");

    AtomDBSingleton::get_instance()->delete_atoms(proxy1->get_built_atoms());
    AtomDBSingleton::get_instance()->delete_atoms(proxy2->get_built_atoms());

    finish_test_case(test_case, success);
    return success;
}

static bool test_customizable_counts() {
    string test_case = start_test_case("test_customizable_counts()");
    bool success = true;

    // clang-format off
    vector<string> query_tokens = {
        AND_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION_TAG,
                LINK, EXPRESSION, "2",
                    NODE, SYMBOL, PREDICATE_TAG,
                    NODE, SYMBOL, "\"contains_bbb\"",
                VARIABLE, "v1",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION_TAG,
                LINK, EXPRESSION, "2",
                    NODE, SYMBOL, PREDICATE_TAG,
                    NODE, SYMBOL, "\"contains_ccc\"",
                VARIABLE, "v2",
    };
    vector<string> count_query_tokens_A = {
        AND_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION_TAG,
                LINK_TEMPLATE, EXPRESSION, "2",
                    NODE, SYMBOL, PREDICATE_TAG,
                    VARIABLE, "Node",
                ATOM, "QueryAnswerElement($v1)",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION_TAG,
                LINK_TEMPLATE, EXPRESSION, "2",
                    NODE, SYMBOL, PREDICATE_TAG,
                    NODE, SYMBOL, "\"sort\"",
                ATOM, "QueryAnswerElement($v1)",
    };
    vector<string> count_query_tokens_B = {
        AND_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION_TAG,
                LINK_TEMPLATE, EXPRESSION, "2",
                    NODE, SYMBOL, PREDICATE_TAG,
                    VARIABLE, "Node",
                ATOM, "QueryAnswerElement($v2)",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION_TAG,
                LINK_TEMPLATE, EXPRESSION, "2",
                    NODE, SYMBOL, PREDICATE_TAG,
                    NODE, SYMBOL, "\"sort\"",
                ATOM, "QueryAnswerElement($v2)",
    };
    // clang-format on
    vector<string> queries = {Utils::join(count_query_tokens_A), Utils::join(count_query_tokens_B)};

    CustomizableLinkCreator link_creator[3];
    link_creator[0].add_link_specification({QueryAnswerElement("v1"), QueryAnswerElement("v2")},
                                           {QueryAnswerElement("Node"), QueryAnswerElement("Node")},
                                           "FunctionalTest0",
                                           CustomizableLinkCreator::INTERSECTION_OVER_UNION,
                                           queries);
    link_creator[1].add_link_specification({QueryAnswerElement("v1"), QueryAnswerElement("v2")},
                                           {QueryAnswerElement("Node"), QueryAnswerElement("Node")},
                                           "FunctionalTest1",
                                           CustomizableLinkCreator::INTERSECTION_OVER_A,
                                           queries);
    link_creator[2].add_link_specification({QueryAnswerElement("v1"), QueryAnswerElement("v2")},
                                           {QueryAnswerElement("Node"), QueryAnswerElement("Node")},
                                           "FunctionalTest2",
                                           CustomizableLinkCreator::INTERSECTION_OVER_B,
                                           queries);

    CustomizableLinkCreator::StrengthComposition composition[3] = {
        CustomizableLinkCreator::INTERSECTION_OVER_UNION,
        CustomizableLinkCreator::INTERSECTION_OVER_A,
        CustomizableLinkCreator::INTERSECTION_OVER_B};

    vector<string> tokens;
    shared_ptr<LinkCreationProxy> proxy[3];
    for (unsigned int i = 0; i < 3; i++) {
        tokens.clear();
        link_creator[i].tokenize(tokens);
        proxy[i] = make_proxy(query_tokens, LinkCreatorRegistry::CUSTOMIZABLE);
        proxy[i]->parameters[LinkCreationProxy::LINK_CREATOR_EXTRA_PARAMETERS] =
            (string) Utils::join(tokens, ',');
        proxy[i]->parameters[LinkCreationProxy::MAX_SUCCESSFUL_CREATION_PER_ROUND] = (unsigned int) 10;
        proxy[i]->parameters[LinkCreationProxy::LINK_CREATION_STRENGTH_THRESHOLD] = (double) 0.001;
        ServiceBusSingleton::get_instance()->issue_bus_command(proxy[i]);
    }

    for (unsigned int i = 0; i < 3; i++) {
        unsigned int count_answers = 0;
        while (true) {
            if (proxy[i]->finished()) {
                break;
            }
            shared_ptr<QueryAnswer> answer = proxy[i]->pop();
            if (answer != nullptr) {
                string handle = proxy[i]->get_built_atoms()[count_answers++];
                double strength = get_strength(handle);
                double expected_strength =
                    compute_expected_strength(answer->get("v1"), answer->get("v2"), composition[i]);
                LOG_DEBUG("[" << strength << ", " << expected_strength << "] "
                              << AtomDBUtils::handle_to_metta(handle));
                success &= assert_true(Utils::is_zero(strength - expected_strength),
                                       "link strength composition: " + std::to_string(i));
            } else {
                Utils::sleep();
            }
        }
        success &= assert_equal(proxy[i]->get_count(), 10, "link creation count");
        AtomDBSingleton::get_instance()->delete_atoms(proxy[i]->get_built_atoms());
    }
    finish_test_case(test_case, success);
    return success;
}

static bool test_cycles() {
    STACK_TRACE();

    string test_case = start_test_case("test_cycles()");
    bool success = true;

    unsigned int CYCLES = 6;

    vector<unsigned int> creations_per_cycle = {100, 200, 300, 400, 500};
    vector<unsigned int> num_cycles = {5, 4, 3, 2, 1};
    vector<unsigned int> total_creation = {0, 0, 0, 0, 0};
    vector<unsigned int> creation = {0, 0, 0, 0, 0};
    vector<shared_ptr<LinkCreationProxy>> proxy = {nullptr, nullptr, nullptr, nullptr, nullptr};

    // clang-format off
    vector<string> query_tokens = {
        AND_OPERATOR, "2",
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION_TAG,
                VARIABLE, PREDICATE1,
                VARIABLE, CONCEPT,
            LINK_TEMPLATE, EXPRESSION, "3",
                NODE, SYMBOL, EVALUATION_TAG,
                VARIABLE, PREDICATE2,
                VARIABLE, CONCEPT
    };
    // clang-format on

    for (unsigned int i = 0; i < proxy.size(); i++) {
        proxy[i] = make_proxy(
            query_tokens, LinkCreatorRegistry::AND_TWO_PREDICATES, BaseProxy::SYNC_ON_CYCLE_START);
        proxy[i]->parameters[LinkCreationProxy::MAX_SUCCESSFUL_CREATION_PER_ROUND] =
            (unsigned int) creations_per_cycle[i];
        proxy[i]->parameters[LinkCreationProxy::MAX_ROUNDS] = (unsigned int) num_cycles[i];
        proxy[i]->parameters[BaseProxy::ORCHESTRATION_SCHEMA] =
            (unsigned int) BaseProxy::SYNC_ON_CYCLE_START;
        ServiceBusSingleton::get_instance()->issue_bus_command(proxy[i]);
    }

    for (unsigned int j = 0; j < CYCLES; j++) {
        for (unsigned int i = 0; i < proxy.size(); i++) {
            if (j < num_cycles[i]) {
                proxy[i]->allow_cycle_start();
            }
            while (true) {
                if (proxy[i]->finished() || proxy[i]->finished_cycle()) {
                    break;
                }
                shared_ptr<QueryAnswer> answer = proxy[i]->pop();
                if (answer == nullptr) {
                    Utils::sleep();
                } else {
                    creation[i]++;
                    total_creation[i]++;
                }
            }
            if (j < num_cycles[i]) {
                success &=
                    assert_equal(creation[i],
                                 creations_per_cycle[i],
                                 "creations in proxy[" + to_string(i) + "] at cycle " + to_string(j));
            } else {
                success &= assert_equal(
                    creation[i], 0, "creations in proxy[" + to_string(i) + "] at cycle " + to_string(j));
            }
            creation[i] = 0;
        }
    }
    for (unsigned int i = 0; i < proxy.size(); i++) {
        while (!proxy[i]->finished()) {
            Utils::sleep();
        }
    }
    for (unsigned int i = 0; i < proxy.size(); i++) {
        success &= assert_equal(total_creation[i],
                                creations_per_cycle[i] * num_cycles[i],
                                "total creations in proxy[" + to_string(i) + "]");
        AtomDBSingleton::get_instance()->delete_atoms(proxy[i]->get_built_atoms());
    }
    finish_test_case(test_case, success);
    return success;
}

int main(int argc, char* argv[]) {
    STACK_TRACE();

    string config_file = "/opt/das/config/das.json";
    auto json_config = JsonConfigParser::load(config_file);
    string client_endpoint = "localhost:35700";
    string server_endpoint = json_config.at_path("agents.query.endpoint").get<string>();
    pair<unsigned int, unsigned int> ports_range = {35701, 35799};

    auto atomdb_config = json_config.at_path("atomdb").get_or<JsonConfig>(JsonConfig());
    SystemParametersSingleton::init(json_config);
    AtomDBSingleton::init(atomdb_config);
    LinkCreatorRegistry::initialize_statics();
    ServiceBusSingleton::init(client_endpoint, server_endpoint, ports_range.first, ports_range.second);

    insert_type_symbols();
    bool success = true;
    timeout_after_minutes(20);
    success &= test_customizable();
    success &= test_customizable_counts();
    success &= test_and_two_predicates();
    success &= test_cycles();
    LOG_INFO("================================================================================");
    if (success) {
        LOG_INFO("OK - ALL TEST CASES PASSED");
        return 0;
    } else {
        LOG_INFO("FAILED - AT LEAST ONE TEST CASE FAILED");
        return 1;
    }
}
