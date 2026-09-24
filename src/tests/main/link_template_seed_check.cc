#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "And.h"
#include "AtomDBSingleton.h"
#include "InMemoryDB.h"
#include "Iterator.h"
#include "Link.h"
#include "LinkTemplate.h"
#include "Node.h"
#include "Or.h"
#include "QueryAnswer.h"
#include "Terminal.h"
#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace atomdb;
using namespace atoms;
using namespace query_element;

static unsigned int parse_seed(int argc, char* argv[]) {
    if (argc == 1) {
        return 0;
    }
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " [--seed=<n>]" << endl;
        cerr << "  seed 0 (default): shuffle equal-importance matches" << endl;
        cerr << "  any other seed:   stable handle order, repeated runs match" << endl;
        exit(1);
    }
    const string arg = argv[1];
    const string prefix = "--seed=";
    if (arg.compare(0, prefix.size(), prefix) != 0) {
        cerr << "Usage: " << argv[0] << " [--seed=<n>]" << endl;
        exit(1);
    }
    return Utils::string_to_uint(arg.substr(prefix.size()));
}

static string join_names(const vector<string>& names) {
    string joined;
    for (unsigned int i = 0; i < names.size(); i++) {
        if (i > 0) {
            joined += " ";
        }
        joined += names[i];
    }
    return joined.empty() ? "<none>" : joined;
}

static shared_ptr<LinkTemplate> make_link_template(const string& predicate_handle,
                                                   const string& anchor_handle,
                                                   const string& variable_name) {
    auto predicate = make_shared<Terminal>();
    predicate->handle = predicate_handle;
    auto anchor = make_shared<Terminal>();
    anchor->handle = anchor_handle;
    auto variable = make_shared<Terminal>(variable_name);
    auto link_template =
        make_shared<LinkTemplate>("Expression",
                                  vector<shared_ptr<QueryElement>>{predicate, anchor, variable},
                                  "",
                                  0.0,
                                  false,
                                  false,
                                  false);
    link_template->build();
    return link_template;
}

static vector<string> collect(shared_ptr<QueryElement> root,
                              const string& tag,
                              const function<string(QueryAnswer*)>& handle_key) {
    Iterator iterator(root);
    vector<string> fetched;
    unsigned int spins = 0;
    while (spins < 100) {
        QueryAnswer* answer = iterator.pop();
        if (answer == nullptr) {
            if (iterator.finished()) {
                break;
            }
            Utils::sleep(100);
            spins++;
            continue;
        }
        spins = 0;
        fetched.push_back(handle_key(answer));
        delete answer;
    }
    LOG_INFO(tag + "_ORDER: " + join_names(fetched));
    return fetched;
}

static unsigned int diff_count(const vector<string>& baseline, const vector<string>& other) {
    unsigned int shared = min(baseline.size(), other.size());
    unsigned int count = 0;
    for (unsigned int i = 0; i < shared; i++) {
        if (baseline[i] != other[i]) {
            count++;
        }
    }
    count +=
        baseline.size() > other.size() ? baseline.size() - other.size() : other.size() - baseline.size();
    return count;
}

static unsigned int report_diffs(const string& tag, const vector<vector<string>>& passes) {
    unsigned int total = 0;
    for (unsigned int pass = 1; pass < passes.size(); pass++) {
        unsigned int diffs = diff_count(passes[0], passes[pass]);
        total += diffs;
        LOG_INFO(tag + " pass " + to_string(pass + 1) + " vs pass 1 DIFF_COUNT: " + to_string(diffs));
    }
    LOG_INFO(tag + " DIFF_COUNT: " + to_string(total));
    return total;
}

int main(int argc, char* argv[]) {
    unsigned int seed = parse_seed(argc, argv);
    Utils::init_random(seed);
    LOG_INFO("SEED: " + to_string(seed) +
             " reproducible: " + string(Utils::reproducible_seed() ? "true" : "false"));
    LOG_INFO(
        "Attention broker must be running. Unknown handles have importance 0, so every match ties.");

    auto db = make_shared<InMemoryDB>();
    AtomDBSingleton::provide(db);

    auto similarity = make_shared<Node>("Symbol", "Similarity");
    auto friendship = make_shared<Node>("Symbol", "Friendship");
    auto anchor = make_shared<Node>("Symbol", "\"anchor\"");
    db->add_node(similarity.get());
    db->add_node(friendship.get());
    db->add_node(anchor.get());

    map<string, string> name_by_handle;
    auto add_concepts = [&](const shared_ptr<Node>& predicate, const vector<string>& names) {
        vector<pair<string, string>> handle_order;
        for (const string& name : names) {
            auto concept = make_shared<Node>("Symbol", "\"" + name + "\"");
            db->add_node(concept.get());
            name_by_handle[concept->handle()] = name;
            handle_order.push_back({concept->handle(), name});
            db->add_link(
                new Link("Expression", {predicate->handle(), anchor->handle(), concept->handle()}));
        }
        sort(handle_order.begin(), handle_order.end());
        vector<string> ordered_names;
        for (const auto& pair : handle_order) {
            ordered_names.push_back(pair.second);
        }
        return join_names(ordered_names);
    };

    string similarity_order =
        add_concepts(similarity, {"mango", "apple", "cherry", "date", "banana", "fig", "kiwi", "grape"});
    string friendship_order = add_concepts(friendship, {"plum", "pear", "peach", "lime"});
    db->re_index_patterns(true);
    LOG_INFO("SIMILARITY_HANDLE_ORDER: " + similarity_order);
    LOG_INFO("FRIENDSHIP_HANDLE_ORDER: " + friendship_order);

    auto name_of = [&](const string& handle) {
        auto found = name_by_handle.find(handle);
        return found == name_by_handle.end() ? handle : found->second;
    };
    auto labeled = [&](const string& handle) { return name_of(handle) + ":" + handle; };

    struct PassOrders {
        vector<string> link_template;
        vector<string> and_query;
        vector<string> or_query;
    };
    auto run_pass = [&](unsigned int pass) {
        LOG_INFO("==== PASS " + to_string(pass) + " ====");
        PassOrders orders;

        auto similarity_template = make_link_template(similarity->handle(), anchor->handle(), "v1");
        LOG_INFO("---- LINK_TEMPLATE ----");
        orders.link_template = collect(
            similarity_template->get_source_element(), "LINK_TEMPLATE", [&](QueryAnswer* answer) {
                return labeled(answer->assignment.get("v1"));
            });

        auto similarity_for_and = make_link_template(similarity->handle(), anchor->handle(), "v1");
        auto friendship_for_and = make_link_template(friendship->handle(), anchor->handle(), "v2");
        auto and_operator = make_shared<And<2>>(
            array<shared_ptr<QueryElement>, 2>{similarity_for_and->get_source_element(),
                                               friendship_for_and->get_source_element()},
            vector<shared_ptr<QueryElement>>{similarity_for_and, friendship_for_and});
        LOG_INFO("---- AND ----");
        orders.and_query = collect(and_operator, "AND", [&](QueryAnswer* answer) {
            return labeled(answer->assignment.get("v1")) + "+" + labeled(answer->assignment.get("v2"));
        });

        auto similarity_for_or = make_link_template(similarity->handle(), anchor->handle(), "left");
        auto friendship_for_or = make_link_template(friendship->handle(), anchor->handle(), "right");
        auto or_operator = make_shared<Or<2>>(
            array<shared_ptr<QueryElement>, 2>{similarity_for_or->get_source_element(),
                                               friendship_for_or->get_source_element()},
            vector<shared_ptr<QueryElement>>{similarity_for_or, friendship_for_or});
        LOG_INFO("---- OR ----");
        orders.or_query = collect(or_operator, "OR", [&](QueryAnswer* answer) {
            string left = answer->assignment.get("left");
            if (!left.empty()) {
                return "similarity:" + labeled(left);
            }
            return "friendship:" + labeled(answer->assignment.get("right"));
        });
        return orders;
    };

    const unsigned int pass_count = 3;
    vector<PassOrders> passes;
    for (unsigned int pass = 1; pass <= pass_count; pass++) {
        passes.push_back(run_pass(pass));
        if (passes.back().link_template.empty() || passes.back().and_query.empty() ||
            passes.back().or_query.empty()) {
            LOG_ERROR("A pass returned no answers. Start the attention broker and run again.");
            return 1;
        }
    }

    vector<vector<string>> link_template_passes;
    vector<vector<string>> and_passes;
    vector<vector<string>> or_passes;
    for (const auto& pass : passes) {
        link_template_passes.push_back(pass.link_template);
        and_passes.push_back(pass.and_query);
        or_passes.push_back(pass.or_query);
    }
    LOG_INFO("==== DIFF ====");
    unsigned int total = 0;
    total += report_diffs("LINK_TEMPLATE", link_template_passes);
    total += report_diffs("AND", and_passes);
    total += report_diffs("OR", or_passes);
    LOG_INFO("DIFF_COUNT: " + to_string(total));
    if (Utils::reproducible_seed()) {
        LOG_INFO("Nonzero seed: DIFF_COUNT should be 0.");
    } else {
        LOG_INFO("Seed 0: DIFF_COUNT should be greater than 0.");
    }
    return 0;
}
