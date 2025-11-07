/**
 * @file LinkProcessor.h
 * @brief Link Processor class
 */

#pragma once
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "Link.h"
#define LOG_LEVEL DEBUG_LEVEL
#include "AtomDBSingleton.h"
#include "Logger.h"
#include "PatternMatchingQueryProxy.h"
#include "QueryAnswer.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"

#define TARGETS_SIZE 2

using namespace atoms;
using namespace query_engine;
using namespace commons;
using namespace std;

namespace link_creation_agent {
class LinkProcessor {
   public:
    LinkProcessor() = default;
    virtual vector<shared_ptr<Link>> process_query(shared_ptr<QueryAnswer> query_answer,
                                                   optional<vector<string>> extra_params = nullopt) = 0;
    virtual ~LinkProcessor() = default;
    static int count_query(vector<string>& query, string& context, bool is_unique_assignment = true) {
        int count = _count_query(query, context, is_unique_assignment);
        // Utils::sleep(500);  // TODO fix this, waiting to delete to not crash
        return count;
    }

    static int _count_query(vector<string>& query, string& context, bool is_unique_assignment = true) {
        try {
            shared_ptr<PatternMatchingQueryProxy> query_count_proxy =
                make_shared<PatternMatchingQueryProxy>(query, context);
            query_count_proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = is_unique_assignment;
            query_count_proxy->parameters[PatternMatchingQueryProxy::COUNT_FLAG] = true;
            ServiceBusSingleton::get_instance()->issue_bus_command(query_count_proxy);
            while (!query_count_proxy->finished()) {
                Utils::sleep(20);
            }
            return query_count_proxy->get_count();
        } catch (const exception& e) {
            LOG_ERROR("Exception: " << e.what());
            return 0;
        }
    }

    static void insert_or_update(map<string, double>& count_map, const string& key, double value) {
        auto iterator = count_map.find(key);
        if (iterator == count_map.end()) {
            count_map[key] = value;
        } else {
            if (value > iterator->second) {
                count_map[key] = value;
            }
        }
    }

    static void compute_counts(const vector<vector<string>>& queries,
                               const string& context,
                               const QueryAnswerElement& target_element,
                               vector<double>& counts,
                               double& count_intersection,
                               double& count_union) {
        if (queries.size() == 0 || queries.size() > TARGETS_SIZE) {
            counts = {};
            count_intersection = 0;
            count_union = 0;
            Utils::error("Invalid number of queries for compute_counts");
        }
        vector<shared_ptr<PatternMatchingQueryProxy>> query_proxies;
        for (const auto& query : queries) {
            counts.push_back(0);
            try {
                shared_ptr<PatternMatchingQueryProxy> query_count_proxy =
                    make_shared<PatternMatchingQueryProxy>(query, context);
                query_count_proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = true;
                query_proxies.push_back(query_count_proxy);
            } catch (const exception& e) {
                Utils::error("Exception: " + string(e.what()));
            }
        }

        map<string, double> count_map[TARGETS_SIZE];
        map<string, double> count_map_union;
        map<string, double> count_map_intersection;
        shared_ptr<QueryAnswer> query_answer;
        auto db = AtomDBSingleton::get_instance();
        double d;
        string handle;
        for (unsigned int i = 0; i < TARGETS_SIZE; i++) {
            ServiceBusSingleton::get_instance()->issue_bus_command(query_proxies[i]);
            while (!query_proxies[i]->finished()) {
                if ((query_answer = query_proxies[i]->pop()) == NULL) {
                    Utils::sleep(20);
                } else {
                    if (query_answer->handles.size() == 1) {
                        d = 1.0;
                    } else {
                        string link_handle = query_answer->handles[1];
                        auto link = db->get_atom(link_handle);
                        d = link->custom_attributes.get<double>("strength");
                    }
                    handle = query_answer->get(target_element);
                    insert_or_update(count_map[i], handle, d);
                }
            }
        }
        count_map_union = count_map[0];
        for (auto pair : count_map[1]) {
            auto iterator = count_map[0].find(pair.first);
            if (iterator == count_map[0].end()) {
                insert_or_update(count_map_union, pair.first, pair.second);
            } else {
                insert_or_update(count_map_union, pair.first, pair.second);
                insert_or_update(count_map_intersection, pair.first, pair.second);
            }
        }

        for (auto pair : count_map_intersection) {
            count_intersection += pair.second;
        }
        for (auto pair : count_map_union) {
            count_union += pair.second;
        }
        for (int i = 0; i < TARGETS_SIZE; i++) {
            for (auto pair : count_map[i]) {
                counts[i] += pair.second;
            }
        }
    }

    static LinkSchema build_pattern_union_query(const string& handle1, const string& handle2) {
        // clang-format off
    vector<string> tokens = {
        "LINK_TEMPLATE", "OR", "2",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "Evaluation",
                "LINK_TEMPLATE", "Expression", "2",
                    "NODE", "Symbol", "Predicate",
                    "VARIABLE", "P",
                "LINK", "Expression", "2",
                    "NODE", "Symbol", "Concept",
                    "ATOM", handle1,
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "Evaluation",
                "LINK_TEMPLATE", "Expression", "2",
                    "NODE", "Symbol", "Predicate",
                    "VARIABLE", "P",
                "LINK", "Expression", "2",
                    "NODE", "Symbol", "Concept",
                    "ATOM", handle2
                    
    };
        // clang-format on
        return LinkSchema(tokens);
    }

    static LinkSchema build_satisfying_set_query(const string& handle1, const string& handle2) {
        // clang-format off
    vector<string> tokens = {
        "LINK_TEMPLATE", "AND", "2", 
            "LINK_TEMPLATE", "Expression", "3", 
                "NODE", "Symbol","Evaluation", 
                "LINK", "Expression", "2", 
                    "NODE", "Symbol", "Predicate",
                    "ATOM", handle1, 
                "LINK_TEMPLATE", "Expression", "2",
                    "NODE", "Symbol", "Concept",
                    "VARIABLE", "C",
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "Evaluation",
                "LINK", "Expression", "2",
                    "NODE", "Symbol", "Predicate",
                    "ATOM", handle2,
                "LINK_TEMPLATE", "Expression", "2",
                    "NODE", "Symbol", "Concept",
                    "VARIABLE", "C"
    };
        // clang-format on
        return LinkSchema(tokens);
    }

    static LinkSchema build_pattern_set_query(const string& handle1, const string& handle2) {
        // clang-format off
    vector<string> tokens = {
        "LINK_TEMPLATE", "AND", "2", 
            "LINK_TEMPLATE", "Expression", "3", 
                "NODE", "Symbol","Evaluation", 
                "LINK_TEMPLATE", "Expression", "2", 
                    "NODE", "Symbol", "Predicate",
                    "VARIABLE", "P",   
                "LINK", "Expression", "2",
                    "NODE", "Symbol", "Concept",
                    "ATOM", handle1,
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "Evaluation",
                "LINK_TEMPLATE", "Expression", "2",
                    "NODE", "Symbol", "Predicate",
                    "VARIABLE", "P",  
                "LINK", "Expression", "2",
                    "NODE", "Symbol", "Concept",
                    "ATOM", handle2
    };
        // clang-format on
        return LinkSchema(tokens);
    }
};

enum class ProcessorType { IMPLICATION_RELATION, EQUIVALENCE_RELATION, INVALID, TEMPLATE, METTA };

class LinkCreationProcessor {
   public:
    static ProcessorType get_processor_type(string processor) {
        if (processor == "IMPLICATION_RELATION") {
            return ProcessorType::IMPLICATION_RELATION;
        } else if (processor == "EQUIVALENCE_RELATION") {
            return ProcessorType::EQUIVALENCE_RELATION;
        } else if (processor == "METTA") {
            return ProcessorType::METTA;
        } else if (processor == "TEMPLATE") {
            return ProcessorType::TEMPLATE;
        } else {
            return ProcessorType::INVALID;
        }
    }
    static string get_processor_type(ProcessorType processor) {
        switch (processor) {
            case ProcessorType::IMPLICATION_RELATION:
                return "IMPLICATION_RELATION";
            case ProcessorType::EQUIVALENCE_RELATION:
                return "EQUIVALENCE_RELATION";
            case ProcessorType::METTA:
                return "METTA";
            case ProcessorType::TEMPLATE:
                return "TEMPLATE";
            default:
                return "INVALID";
        }
    }
};
}  // namespace link_creation_agent
