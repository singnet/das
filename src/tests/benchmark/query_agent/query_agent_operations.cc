#include "query_agent_operations.h"

#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "query_agent_runner.h"

void PatternMatchingQuery::minimal_query(string log_file) {
    vector<double> operation_time_minimal_query_client_side;

    for (int i = 0; i < iterations_; ++i) {
        vector<string> q1 = {
            "LINK_TEMPLATE", "Expression", "2", "NODE", "Symbol", "Sentence", "VARIABLE", "S"};

        operation_time_minimal_query_client_side.push_back(measure_execution_time([&]() {
            auto proxy = atom_space_->pattern_matching_query(
                q1, IGNORE_ANSWER_COUNT, "", false, true, false, false, false);
            int count = 0;
            while (!proxy->finished()) {
                if (proxy->pop() == NULL) {
                    Utils::sleep();
                } else {
                    if (count++ > 1000) {
                        break;
                    }
                }
            }
        }));
    }

    double total_time_client_side = accumulate(operation_time_minimal_query_client_side.begin(),
                                               operation_time_minimal_query_client_side.end(),
                                               0.0);
    double avg_time_client_side = total_time_client_side / iterations_;
    double ops_per_sec_client_side = iterations_ / (total_time_client_side / 1000.0);
    global_metrics["minimal_query_client_side"] = Metrics{operation_time_minimal_query_client_side,
                                                          total_time_client_side,
                                                          avg_time_client_side,
                                                          ops_per_sec_client_side};

    vector<double> operation_time_minimal_query_server_side =
        parse_server_side_benchmark_times(log_file);

    double total_time_server_side = accumulate(operation_time_minimal_query_server_side.begin(),
                                               operation_time_minimal_query_server_side.end(),
                                               0.0);
    double avg_time_server_side = total_time_server_side / iterations_;
    double ops_per_sec_server_side = iterations_ / (total_time_server_side / 1000.0);
    global_metrics["minimal_query_server_side"] = Metrics{operation_time_minimal_query_server_side,
                                                          total_time_server_side,
                                                          avg_time_server_side,
                                                          ops_per_sec_server_side};
}

void PatternMatchingQuery::positive_importance() {
    vector<double> operation_time_positive_importance_false;
    vector<double> operation_time_positive_importance_true;

    for (int i = 0; i < iterations_; ++i) {
        vector<string> q1 = {
            "LINK_TEMPLATE", "Expression", "2", "NODE", "Symbol", "Word", "VARIABLE", "W"};

        auto proxy = atom_space_->pattern_matching_query(q1);
        shared_ptr<QueryAnswer> query_answer;
        vector<string> node_names;

        int count = 0;
        while (!proxy->finished()) {
            if ((query_answer = proxy->pop()) == NULL) {
                Utils::sleep();
            } else {
                if (count++ == 2) {
                    break;
                }
                auto word_node_handle = query_answer->assignment.get("W");
                shared_ptr<AtomDB> atomdb = AtomDBSingleton::get_instance();
                auto atom_document = atomdb->get_atom_document(word_node_handle);
                auto node_name = atom_document->get("name");
                node_names.push_back(string(node_name));
            }
        }

        // clang-format off
        vector<string> q2 = {
            "AND", "2",
                "LINK_TEMPLATE", "Expression", "3",
                    "NODE", "Symbol", "Contains",
                    "VARIABLE", "S1",
                    "LINK", "Expression", "2",
                        "NODE", "Symbol", "Word",
                        "NODE", "Symbol", node_names[0],
                "LINK_TEMPLATE", "Expression", "3",
                    "NODE", "Symbol", "Contains",
                    "VARIABLE", "S2",
                    "LINK", "Expression", "2",
                        "NODE", "Symbol", "Word",
                        "NODE", "Symbol", node_names[1]
        };
        // clang-format on

        auto proxy2 = atom_space_->pattern_matching_query(
            q2, IGNORE_ANSWER_COUNT, "", false, true, true, false, false);
        while (!proxy2->finished()) {
            if (proxy2->pop() == NULL) {
                Utils::sleep();
            }
        }
        // ----- benchmark code -----

        // clang-format off
        vector<string> q3 = {
            "LINK_TEMPLATE", "Expression", "3",
                "NODE", "Symbol", "Contains",
                "VARIABLE", "S",
                "VARIABLE", "W"
            };
        // clang-format on  

        operation_time_positive_importance_false.push_back(measure_execution_time([&]() {
            auto proxy = atom_space_->pattern_matching_query(q3, IGNORE_ANSWER_COUNT, "", false, true, false, false, false);
            while (!proxy->finished()) {
                if (proxy->pop() == NULL) {
                    Utils::sleep();
                }
            }
        }));

        operation_time_positive_importance_true.push_back(measure_execution_time([&]() {
            auto proxy = atom_space_->pattern_matching_query(q3, IGNORE_ANSWER_COUNT, "", false, true, false, true, false);
            while (!proxy->finished()) {
                if (proxy->pop() == NULL) {
                    Utils::sleep();
                }
            }
        }));
    }

    double total_time = accumulate(operation_time_positive_importance_false.begin(), operation_time_positive_importance_false.end(), 0.0);
    double avg_time = total_time / iterations_;
    double ops_per_sec = iterations_ / (total_time / 1000.0);

    global_mutex.lock();
    global_metrics["positive_importance_false_client_side"] = Metrics{operation_time_positive_importance_false, total_time, avg_time, ops_per_sec};
    global_mutex.unlock();

    double total_time1 = accumulate(operation_time_positive_importance_true.begin(), operation_time_positive_importance_true.end(), 0.0);
    double avg_time1 = total_time1 / iterations_;
    double ops_per_sec1 = iterations_ / (total_time1 / 1000.0);

    global_mutex.lock();
    global_metrics["positive_importance_true_client_side"] = Metrics{operation_time_positive_importance_true, total_time1, avg_time1, ops_per_sec1};
    global_mutex.unlock();
}

void PatternMatchingQuery::complex_query(string log_file) {

    vector<double> operation_time_complex_query_client_side;

    for (int i = 0; i < iterations_; ++i) {
        vector<string> q1 = {"LINK_TEMPLATE", "Expression", "2", "NODE", "Symbol", "Word", "VARIABLE", "W"};

        auto proxy = atom_space_->pattern_matching_query(q1);
        shared_ptr<QueryAnswer> query_answer;
        vector<string> node_names;

        while (!proxy->finished()) {
            if ((query_answer = proxy->pop()) == NULL) {
                Utils::sleep();
            } else {
                if (node_names.size() >= 10) {
                    break;
                }
                auto word_node_handle = query_answer->assignment.get("W");
                shared_ptr<AtomDB> atomdb = AtomDBSingleton::get_instance();
                auto atom_document = atomdb->get_atom_document(word_node_handle);
                auto node_name = atom_document->get("name");
                if (find(node_names.begin(), node_names.end(), string(node_name)) == node_names.end()) {
                    node_names.push_back(string(node_name));
                } 
            }
        }

        // clang-format off
        vector<string> q2 = {
            "OR", "2",
                "AND", "5", 
                    "LINK_TEMPLATE", "Expression", "3",
                        "NODE", "Symbol", "Contains",
                        "LINK_TEMPLATE", "Expression", "2", "NODE", "Symbol", "Sentence", "VARIABLE", "S",
                        "LINK", "Expression", "2", "NODE", "Symbol", "Word", "NODE", "Symbol", node_names[0],
                    "LINK_TEMPLATE", "Expression", "3",
                        "NODE", "Symbol", "Contains",
                        "LINK_TEMPLATE", "Expression", "2", "NODE", "Symbol", "Sentence", "VARIABLE", "S",
                        "LINK", "Expression", "2", "NODE", "Symbol", "Word", "NODE", "Symbol", node_names[1],
                    "LINK_TEMPLATE", "Expression", "3",
                        "NODE", "Symbol", "Contains",
                        "LINK_TEMPLATE", "Expression", "2", "NODE", "Symbol", "Sentence", "VARIABLE", "S",
                        "LINK", "Expression", "2", "NODE", "Symbol", "Word", "NODE", "Symbol", node_names[2],
                    "LINK_TEMPLATE", "Expression", "3",
                        "NODE", "Symbol", "Contains",
                        "LINK_TEMPLATE", "Expression", "2", "NODE", "Symbol", "Sentence", "VARIABLE", "S",
                        "LINK", "Expression", "2", "NODE", "Symbol", "Word", "NODE", "Symbol", node_names[3],
                    "LINK_TEMPLATE", "Expression", "3",
                        "NODE", "Symbol", "Contains",
                        "LINK_TEMPLATE", "Expression", "2", "NODE", "Symbol", "Sentence", "VARIABLE", "S",
                        "LINK", "Expression", "2", "NODE", "Symbol", "Word", "NODE", "Symbol", node_names[4],
                "AND", "5", 
                    "LINK_TEMPLATE", "Expression", "3",
                        "NODE", "Symbol", "Contains",
                        "LINK_TEMPLATE", "Expression", "2", "NODE", "Symbol", "Sentence", "VARIABLE", "S",
                        "LINK", "Expression", "2", "NODE", "Symbol", "Word", "NODE", "Symbol", node_names[5],
                    "LINK_TEMPLATE", "Expression", "3",
                        "NODE", "Symbol", "Contains",
                        "LINK_TEMPLATE", "Expression", "2", "NODE", "Symbol", "Sentence", "VARIABLE", "S",
                        "LINK", "Expression", "2", "NODE", "Symbol", "Word", "NODE", "Symbol", node_names[6],
                    "LINK_TEMPLATE", "Expression", "3",
                        "NODE", "Symbol", "Contains",
                        "LINK_TEMPLATE", "Expression", "2", "NODE", "Symbol", "Sentence", "VARIABLE", "S",
                        "LINK", "Expression", "2", "NODE", "Symbol", "Word", "NODE", "Symbol", node_names[7],
                    "LINK_TEMPLATE", "Expression", "3",
                        "NODE", "Symbol", "Contains",
                        "LINK_TEMPLATE", "Expression", "2", "NODE", "Symbol", "Sentence", "VARIABLE", "S",
                        "LINK", "Expression", "2", "NODE", "Symbol", "Word", "NODE", "Symbol", node_names[8],
                    "LINK_TEMPLATE", "Expression", "3",
                        "NODE", "Symbol", "Contains",
                        "LINK_TEMPLATE", "Expression", "2", "NODE", "Symbol", "Sentence", "VARIABLE", "S",
                        "LINK", "Expression", "2", "NODE", "Symbol", "Word", "NODE", "Symbol", node_names[9],
        };
        // clang-format on

        operation_time_complex_query_client_side.push_back(measure_execution_time([&]() {
            auto proxy = atom_space_->pattern_matching_query(
                q2, IGNORE_ANSWER_COUNT, "", false, true, false, false, false);
            int count = 0;
            while (!proxy->finished()) {
                if (proxy->pop() == NULL) {
                    Utils::sleep();
                } else {
                    if (count++ > 1000) {
                        break;
                    }
                }
            }
        }));
    }

    double total_time_client_side = accumulate(operation_time_complex_query_client_side.begin(),
                                               operation_time_complex_query_client_side.end(),
                                               0.0);
    double avg_time_client_side = total_time_client_side / iterations_;
    double ops_per_sec_client_side = iterations_ / (total_time_client_side / 1000.0);
    global_metrics["complex_query_client_side"] = Metrics{operation_time_complex_query_client_side,
                                                          total_time_client_side,
                                                          avg_time_client_side,
                                                          ops_per_sec_client_side};

    // double total_time_server_side = accumulate(operation_time_complex_query_server_side.begin(),
    // operation_time_complex_query_server_side.end(), 0.0); double avg_time_server_side =
    // total_time_server_side / iterations_; double ops_per_sec_server_side = iterations_ /
    // (total_time_server_side / 1000.0); global_metrics["complex_query_server_side"] =
    // Metrics{operation_time_complex_query_server_side, total_time_server_side, avg_time_server_side,
    // ops_per_sec_server_side};
}

void PatternMatchingQuery::update_attention_broker() {
    // WIP
}

void PatternMatchingQuery::link_template_cache() {
    // WIP
}