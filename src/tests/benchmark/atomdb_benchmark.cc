#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#include "AtomDB.h"
#include "AtomDBCacheSingleton.h"
#include "MorkDB.h"
#include "RedisMongoDB.h"
#include "Utils.h"
#include "atomdb_benchmark_utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace atomdb;

using Clock = chrono::steady_clock;
const int BATCH_SIZE = 10;  // Number of atoms in a single batch

mutex global_mutex;
map<string, Metrics> global_metrics;

class AddAtom {
   public:
    AddAtom(int tid, shared_ptr<AtomDB> db, int iterations)
        : tid_(tid), db_(db), iterations_(iterations) {}

    void add_node() {
        vector<double> add_node_operation_time;
        for (int i = 0; i < iterations_; ++i) {
            auto node_a = new Node(
                "Symbol", string("\"NODE_A") + "_t" + to_string(tid_) + "_i" + to_string(i) + "\"");
            auto t0 = Clock::now();
            auto handle = db_->add_node(node_a);
            auto t1 = Clock::now();
            double ms = chrono::duration<double, milli>(t1 - t0).count();
            add_node_operation_time.push_back(ms);
        }
        double nodes_total_time =
            accumulate(add_node_operation_time.begin(), add_node_operation_time.end(), 0.0);
        global_mutex.lock();
        global_metrics["add_node"] = Metrics{add_node_operation_time,
                                             nodes_total_time,
                                             nodes_total_time / iterations_,
                                             iterations_ / (nodes_total_time / 1000.0)};
        global_mutex.unlock();
    };

    void add_link() {
        vector<double> add_link_operation_time;
        for (int i = 0; i < iterations_; i++) {
            string suffix = "_t" + to_string(tid_) + "_i" + to_string(i);
            auto node_equivalence = new Node("Symbol", string("EQUIVALENCE_A") + suffix);
            auto node_equivalence_handle = db_->add_node(node_equivalence);
            auto node_b = new Node("Symbol", string("NODE_B") + suffix);
            auto node_b_handle = db_->add_node(node_b);
            auto node_c = new Node("Symbol", string("NODE_C") + suffix);
            auto node_c_handle = db_->add_node(node_c);
            auto link_a =
                new Link("Expression", {node_equivalence_handle, node_b_handle, node_c_handle});

            auto t0 = Clock::now();
            db_->add_link(link_a);
            auto t1 = Clock::now();

            double ms = chrono::duration<double, milli>(t1 - t0).count();
            add_link_operation_time.push_back(ms);
        }
        double links_total_time =
            accumulate(add_link_operation_time.begin(), add_link_operation_time.end(), 0.0);
        global_mutex.lock();
        global_metrics["add_link"] = Metrics{add_link_operation_time,
                                             links_total_time,
                                             links_total_time / iterations_,
                                             iterations_ / (links_total_time / 1000.0)};
        global_mutex.unlock();
    };

    void add_atom_node() {
        vector<double> add_atom_node_operation_time;
        for (int i = 0; i < iterations_; ++i) {
            auto node_d = new Node(
                "Symbol", string("\"NODE_D") + "_t" + to_string(tid_) + "_i" + to_string(i) + "\"");

            auto t0 = Clock::now();
            db_->add_atom(node_d);
            auto t1 = Clock::now();

            double ms = chrono::duration<double, milli>(t1 - t0).count();
            add_atom_node_operation_time.push_back(ms);
        }
        double atoms_node_total_time =
            accumulate(add_atom_node_operation_time.begin(), add_atom_node_operation_time.end(), 0.0);
        global_mutex.lock();
        global_metrics["add_atom[node]"] = Metrics{add_atom_node_operation_time,
                                                   atoms_node_total_time,
                                                   atoms_node_total_time / iterations_,
                                                   iterations_ / (atoms_node_total_time / 1000.0)};
        global_mutex.unlock();
    };

    void add_atom_link() {
        vector<double> add_atom_link_operation_time;
        for (int i = 0; i < iterations_; i++) {
            string suffix = "_t" + to_string(tid_) + "_i" + to_string(i);
            auto node_equivalence = new Node("Symbol", string("EQUIVALENCE_B") + suffix);
            auto node_equivalence_handle = db_->add_node(node_equivalence);
            auto node_e = new Node("Symbol", string("\"NODE_E") + suffix + "\"");
            auto node_e_handle = db_->add_node(node_e);
            auto node_f = new Node("Symbol", string("\"NODE_F") + suffix + "\"");
            auto node_f_handle = db_->add_node(node_f);
            auto link_b =
                new Link("Expression", {node_equivalence_handle, node_e_handle, node_f_handle});

            auto t0 = Clock::now();
            db_->add_link(link_b);
            auto t1 = Clock::now();

            double ms = chrono::duration<double, milli>(t1 - t0).count();
            add_atom_link_operation_time.push_back(ms);
        }
        double atoms_link_total_time =
            accumulate(add_atom_link_operation_time.begin(), add_atom_link_operation_time.end(), 0.0);
        global_mutex.lock();
        global_metrics["add_atom[link]"] = Metrics{add_atom_link_operation_time,
                                                   atoms_link_total_time,
                                                   atoms_link_total_time / iterations_,
                                                   iterations_ / (atoms_link_total_time / 1000.0)};
        global_mutex.unlock();
    };

   private:
    int tid_;
    shared_ptr<AtomDB> db_;
    int iterations_;
};

class AddAtoms {
   public:
    AddAtoms(int tid, shared_ptr<AtomDB> db, int iterations)
        : tid_(tid), db_(db), iterations_(iterations) {}

    void add_nodes() {
        vector<double> add_nodes_operation_time;
        for (int i = 0; i < iterations_; ++i) {
            vector<Node*> nodes;
            for (int j = 0; j < BATCH_SIZE; j++) {
                nodes.push_back(new Node("Symbol",
                                         string("\"NODES_A") + "_t" + to_string(tid_) + "_i" +
                                             to_string(i) + "_j" + to_string(j) + "\""));
            }
            auto t0 = Clock::now();
            db_->add_nodes(nodes);
            auto t1 = Clock::now();
            double ms = chrono::duration<double, milli>(t1 - t0).count();
            add_nodes_operation_time.push_back(ms);
        }
        double nodes_total_time =
            accumulate(add_nodes_operation_time.begin(), add_nodes_operation_time.end(), 0.0);
        global_mutex.lock();
        global_metrics["add_nodes"] = Metrics{add_nodes_operation_time,
                                              nodes_total_time,
                                              nodes_total_time / (iterations_ * BATCH_SIZE),
                                              (iterations_ * BATCH_SIZE) / (nodes_total_time / 1000.0)};
        global_mutex.unlock();
    }

    void add_links() {
        vector<double> add_links_operation_time;
        for (int i = 0; i < iterations_; i++) {
            vector<Link*> links;
            for (int j = 0; j < BATCH_SIZE; j++) {
                string suffix = "_t" + to_string(tid_) + "_i" + to_string(i) + "_j" + to_string(j);
                auto node_equivalence = new Node("Symbol", string("EQUIVALENCE_A") + suffix);
                auto node_equivalence_handle = db_->add_node(node_equivalence);
                auto node_b = new Node("Symbol", string("\"NODES_B") + suffix + "\"");
                auto node_b_handle = db_->add_node(node_b);
                auto node_c = new Node("Symbol", string("\"NODES_C") + suffix + "\"");
                auto node_c_handle = db_->add_node(node_c);
                links.push_back(
                    new Link("Expression", {node_equivalence_handle, node_b_handle, node_c_handle}));
            }

            auto t0 = Clock::now();
            db_->add_links(links);
            auto t1 = Clock::now();

            double ms = chrono::duration<double, milli>(t1 - t0).count();
            add_links_operation_time.push_back(ms);
        }
        double links_total_time =
            accumulate(add_links_operation_time.begin(), add_links_operation_time.end(), 0.0);
        global_mutex.lock();
        global_metrics["add_links"] = Metrics{add_links_operation_time,
                                              links_total_time,
                                              links_total_time / (iterations_ * BATCH_SIZE),
                                              (iterations_ * BATCH_SIZE) / (links_total_time / 1000.0)};
        global_mutex.unlock();
    }

    void add_atoms_node() {
        vector<double> add_atoms_node_operation_time;
        for (int i = 0; i < iterations_; ++i) {
            vector<Atom*> atoms_node;
            for (int j = 0; j < BATCH_SIZE; j++) {
                atoms_node.push_back(new Node("Symbol",
                                              string("\"NODES_D") + "_t" + to_string(tid_) + "_i" +
                                                  to_string(i) + "_j" + to_string(j) + "\""));
            }
            auto t0 = Clock::now();
            db_->add_atoms(atoms_node);
            auto t1 = Clock::now();
            double ms = chrono::duration<double, milli>(t1 - t0).count();
            add_atoms_node_operation_time.push_back(ms);
        }
        double atoms_node_total_time =
            accumulate(add_atoms_node_operation_time.begin(), add_atoms_node_operation_time.end(), 0.0);
        global_metrics["add_atoms[node]"] =
            Metrics{add_atoms_node_operation_time,
                    atoms_node_total_time,
                    atoms_node_total_time / (iterations_ * BATCH_SIZE),
                    (iterations_ * BATCH_SIZE) / (atoms_node_total_time / 1000.0)};
    }

    void add_atoms_link() {
        vector<double> add_atoms_link_operation_time;

        for (int i = 0; i < iterations_; i++) {
            vector<Atom*> atoms_link;

            for (int j = 0; j < BATCH_SIZE; j++) {
                string suffix = "_t" + to_string(tid_) + "_i" + to_string(i) + "_j" + to_string(j);
                auto node_equivalence = new Node("Symbol", string("EQUIVALENCE_B") + suffix);
                auto node_equivalence_handle = db_->add_node(node_equivalence);
                auto node_e = new Node("Symbol", string("\"NODES_E") + suffix + "\"");
                auto node_e_handle = db_->add_node(node_e);
                auto node_f = new Node("Symbol", string("\"NODES_F") + suffix + "\"");
                auto node_f_handle = db_->add_node(node_f);
                atoms_link.push_back(
                    new Link("Expression", {node_equivalence_handle, node_e_handle, node_f_handle}));
            }

            auto t0 = Clock::now();
            db_->add_atoms(atoms_link);
            auto t1 = Clock::now();

            double ms = chrono::duration<double, milli>(t1 - t0).count();
            add_atoms_link_operation_time.push_back(ms);
        }
        double atoms_link_total_time =
            accumulate(add_atoms_link_operation_time.begin(), add_atoms_link_operation_time.end(), 0.0);
        global_metrics["add_atoms[link]"] =
            Metrics{add_atoms_link_operation_time,
                    atoms_link_total_time,
                    atoms_link_total_time / (iterations_ * BATCH_SIZE),
                    (iterations_ * BATCH_SIZE) / (atoms_link_total_time / 1000.0)};
    }

   private:
    int tid_;
    shared_ptr<AtomDB> db_;
    int iterations_;
};

class GetAtom {
   public:
    GetAtom(int tid, shared_ptr<AtomDB> db, int iterations)
        : tid_(tid), db_(db), iterations_(iterations) {}

    void get_node_document() {
        vector<double> get_node_document_operation_time;
        for (int i = 0; i < iterations_; ++i) {
            auto node_a = new Node(
                "Symbol", string("\"NODE_A") + "_t" + to_string(tid_) + "_i" + to_string(i) + "\"");
            auto node_a_handle = db_->add_node(node_a);

            auto t0 = Clock::now();
            db_->get_node_document(node_a_handle);
            auto t1 = Clock::now();

            double ms = chrono::duration<double, milli>(t1 - t0).count();
            get_node_document_operation_time.push_back(ms);
        }
        double get_node_document_total_time = accumulate(
            get_node_document_operation_time.begin(), get_node_document_operation_time.end(), 0.0);
        global_mutex.lock();
        global_metrics["get_node_document"] =
            Metrics{get_node_document_operation_time,
                    get_node_document_total_time,
                    get_node_document_total_time / iterations_,
                    iterations_ / (get_node_document_total_time / 1000.0)};
        global_mutex.unlock();
    }

    void get_link_document() {
        vector<double> get_link_document_operation_time;
        for (int i = 0; i < iterations_; ++i) {
            string suffix = "_t" + to_string(tid_) + "_i" + to_string(i);
            auto node_equivalence = new Node("Symbol", string("EQUIVALENCE_A") + suffix);
            auto node_equivalence_handle = db_->add_node(node_equivalence);
            auto node_b = new Node("Symbol", string("\"NODE_B") + suffix + "\"");
            auto node_b_handle = db_->add_node(node_b);
            auto node_c = new Node("Symbol", string("\"NODE_C") + suffix + "\"");
            auto node_c_handle = db_->add_node(node_c);

            auto link = new Link("Expression", {node_equivalence_handle, node_b_handle, node_c_handle});
            auto link_handle = db_->add_link(link);

            auto t0 = Clock::now();
            db_->get_link_document(link_handle);
            auto t1 = Clock::now();

            double ms = chrono::duration<double, milli>(t1 - t0).count();
            get_link_document_operation_time.push_back(ms);
        }
        double get_link_document_total_time = accumulate(
            get_link_document_operation_time.begin(), get_link_document_operation_time.end(), 0.0);
        global_mutex.lock();
        global_metrics["get_link_document"] =
            Metrics{get_link_document_operation_time,
                    get_link_document_total_time,
                    get_link_document_total_time / iterations_,
                    iterations_ / (get_link_document_total_time / 1000.0)};
        global_mutex.unlock();
    }

    void get_atom_document_node() {
        vector<double> get_atom_document_node_operation_time;
        for (int i = 0; i < iterations_; ++i) {
            auto node_d = new Node(
                "Symbol", string("\"NODE_D") + "_t" + to_string(tid_) + "_i" + to_string(i) + "\"");
            auto node_d_handle = db_->add_atom(node_d);

            auto t0 = Clock::now();
            db_->get_atom_document(node_d_handle);
            auto t1 = Clock::now();

            double ms = chrono::duration<double, milli>(t1 - t0).count();
            get_atom_document_node_operation_time.push_back(ms);
        }
        double get_atom_document_node_total_time =
            accumulate(get_atom_document_node_operation_time.begin(),
                       get_atom_document_node_operation_time.end(),
                       0.0);
        global_mutex.lock();
        global_metrics["get_atom_document[node]"] =
            Metrics{get_atom_document_node_operation_time,
                    get_atom_document_node_total_time,
                    get_atom_document_node_total_time / iterations_,
                    iterations_ / (get_atom_document_node_total_time / 1000.0)};
        global_mutex.unlock();
    }

    void get_atom_document_link() {
        vector<double> get_atom_document_link_operation_time;
        for (int i = 0; i < iterations_; ++i) {
            string suffix = "_t" + to_string(tid_) + "_i" + to_string(i);
            auto node_equivalence = new Node("Symbol", string("EQUIVALENCE_B") + suffix);
            auto node_equivalence_handle = db_->add_node(node_equivalence);
            auto node_e = new Node("Symbol", string("\"NODE_E") + suffix + "\"");
            auto node_e_handle = db_->add_node(node_e);
            auto node_f = new Node("Symbol", string("\"NODE_F") + suffix + "\"");
            auto node_f_handle = db_->add_node(node_f);

            auto link = new Link("Expression", {node_equivalence_handle, node_e_handle, node_f_handle});

            auto link_handle = db_->add_link(link);

            auto t0 = Clock::now();
            db_->get_atom_document(link_handle);
            auto t1 = Clock::now();

            double ms = chrono::duration<double, milli>(t1 - t0).count();
            get_atom_document_link_operation_time.push_back(ms);
        }
        double get_atom_document_link_total_time =
            accumulate(get_atom_document_link_operation_time.begin(),
                       get_atom_document_link_operation_time.end(),
                       0.0);
        global_mutex.lock();
        global_metrics["get_atom_document[link]"] =
            Metrics{get_atom_document_link_operation_time,
                    get_atom_document_link_total_time,
                    get_atom_document_link_total_time / iterations_,
                    iterations_ / (get_atom_document_link_total_time / 1000.0)};
        global_mutex.unlock();
    }

    void get_atom_node() {
        vector<double> get_atom_node_operation_time;
        for (int i = 0; i < iterations_; ++i) {
            auto node_g = new Node(
                "Symbol", string("\"NODE_G") + "_t" + to_string(tid_) + "_i" + to_string(i) + "\"");
            auto node_g_handle = db_->add_atom(node_g);

            auto t0 = Clock::now();
            db_->get_atom(node_g_handle);
            auto t1 = Clock::now();

            double ms = chrono::duration<double, milli>(t1 - t0).count();
            get_atom_node_operation_time.push_back(ms);
        }
        double get_atom_node_total_time =
            accumulate(get_atom_node_operation_time.begin(), get_atom_node_operation_time.end(), 0.0);
        global_mutex.lock();
        global_metrics["get_atom[node]"] = Metrics{get_atom_node_operation_time,
                                                   get_atom_node_total_time,
                                                   get_atom_node_total_time / iterations_,
                                                   iterations_ / (get_atom_node_total_time / 1000.0)};
        global_mutex.unlock();
    }

    void get_atom_link() {
        vector<double> get_atom_link_operation_time;
        for (int i = 0; i < iterations_; ++i) {
            string suffix = "_t" + to_string(tid_) + "_i" + to_string(i);
            auto node_equivalence = new Node("Symbol", string("EQUIVALENCE_C") + suffix);
            auto node_equivalence_handle = db_->add_node(node_equivalence);
            auto node_g = new Node("Symbol", string("\"NODE_H") + suffix + "\"");
            auto node_g_handle = db_->add_node(node_g);
            auto node_i = new Node("Symbol", string("\"NODE_I") + suffix + "\"");
            auto node_i_handle = db_->add_node(node_i);

            auto link = new Link("Expression", {node_equivalence_handle, node_g_handle, node_i_handle});

            auto link_handle = db_->add_link(link);

            auto t0 = Clock::now();
            db_->get_atom(link_handle);
            auto t1 = Clock::now();

            double ms = chrono::duration<double, milli>(t1 - t0).count();
            get_atom_link_operation_time.push_back(ms);
        }
        double get_atom_link_total_time =
            accumulate(get_atom_link_operation_time.begin(), get_atom_link_operation_time.end(), 0.0);
        global_mutex.lock();
        global_metrics["get_atom[link]"] = Metrics{get_atom_link_operation_time,
                                                   get_atom_link_total_time,
                                                   get_atom_link_total_time / iterations_,
                                                   iterations_ / (get_atom_link_total_time / 1000.0)};
        global_mutex.unlock();
    }

   private:
    int tid_;
    shared_ptr<AtomDB> db_;
    int iterations_;
};

class GetAtoms {
   public:
    GetAtoms(int tid, shared_ptr<AtomDB> db, int iterations)
        : tid_(tid), db_(db), iterations_(iterations) {}

    void get_node_documents() {}

    void get_link_documents() {}

    void get_atom_documents_node() {}

    void get_atom_documents_link() {}

    void query_for_pattern() {
        LinkSchema link_schema({"LINK_TEMPLATE",
                                "Expression",
                                "3",
                                "NODE",
                                "Symbol",
                                "Contains",
                                "VARIABLE",
                                "$sentence",
                                "VARIABLE",
                                "$word"});
        vector<double> query_for_pattern_operation_time;
        for (int i = 0; i < iterations_; ++i) {
            auto t0 = Clock::now();
            auto handles_set = db_->query_for_pattern(link_schema);
            auto iterator = handles_set->get_iterator();
            auto first_handle = iterator->next();
            auto t1 = Clock::now();
            double ms = chrono::duration<double, milli>(t1 - t0).count();
            query_for_pattern_operation_time.push_back(ms);
        }
        double query_for_pattern_total_time = accumulate(
            query_for_pattern_operation_time.begin(), query_for_pattern_operation_time.end(), 0.0);
        global_mutex.lock();
        global_metrics["query_for_pattern[first_result]"] =
            Metrics{query_for_pattern_operation_time,
                    query_for_pattern_total_time,
                    query_for_pattern_total_time / iterations_,
                    iterations_ / (query_for_pattern_total_time / 1000.0)};
        global_mutex.unlock();
    }

    void query_for_targets() {
        for (int i = 0; i < iterations_; ++i) {
            // Query for targets
        }
    }

   private:
    int tid_;
    shared_ptr<AtomDB> db_;
    int iterations_;
};

class DeleteAtom {
   public:
    DeleteAtom(int tid, shared_ptr<AtomDB> db, int iterations)
        : tid_(tid), db_(db), iterations_(iterations) {}

    void delete_node() {
        vector<double> delete_node_operation_time;
        for (int i = 0; i < iterations_; ++i) {
            auto node_a = new Node(
                "Symbol", string("\"NODE_A") + "_t" + to_string(tid_) + "_i" + to_string(i) + "\"");
            auto handle = db_->add_atom(node_a);
            auto t0 = Clock::now();
            db_->delete_node(handle);
            auto t1 = Clock::now();
            double ms = chrono::duration<double, milli>(t1 - t0).count();
            delete_node_operation_time.push_back(ms);
        }
        double delete_node_total_time =
            accumulate(delete_node_operation_time.begin(), delete_node_operation_time.end(), 0.0);
        global_mutex.lock();
        global_metrics["delete_node"] = Metrics{delete_node_operation_time,
                                                delete_node_total_time,
                                                delete_node_total_time / iterations_,
                                                iterations_ / (delete_node_total_time / 1000.0)};
        global_mutex.unlock();
    }

    void delete_link() {
        vector<double> delete_link_operation_time;
        for (int i = 0; i < iterations_; i++) {
            string suffix = "_t" + to_string(tid_) + "_i" + to_string(i);
            auto node_equivalence = new Node("Symbol", string("EQUIVALENCE_A") + suffix);
            auto node_equivalence_handle = db_->add_node(node_equivalence);
            auto node_b = new Node("Symbol", string("NODE_B") + suffix);
            auto node_b_handle = db_->add_node(node_b);
            auto node_c = new Node("Symbol", string("NODE_C") + suffix);
            auto node_c_handle = db_->add_node(node_c);
            auto link_a =
                new Link("Expression", {node_equivalence_handle, node_b_handle, node_c_handle});
            auto link_handle = db_->add_link(link_a);

            auto t0 = Clock::now();
            db_->delete_link(link_handle);
            auto t1 = Clock::now();

            double ms = chrono::duration<double, milli>(t1 - t0).count();
            delete_link_operation_time.push_back(ms);
        }
        double delete_link_total_time =
            accumulate(delete_link_operation_time.begin(), delete_link_operation_time.end(), 0.0);
        global_mutex.lock();
        global_metrics["delete_link"] = Metrics{delete_link_operation_time,
                                                delete_link_total_time,
                                                delete_link_total_time / iterations_,
                                                iterations_ / (delete_link_total_time / 1000.0)};
        global_mutex.unlock();
    }

    void delete_atom_node() {
        vector<double> delete_atom_node_operation_time;
        for (int i = 0; i < iterations_; ++i) {
            auto node_d = new Node(
                "Symbol", string("\"NODE_D") + "_t" + to_string(tid_) + "_i" + to_string(i) + "\"");
            auto node_d_handle = db_->add_atom(node_d);

            auto t0 = Clock::now();
            db_->delete_atom(node_d_handle);
            auto t1 = Clock::now();

            double ms = chrono::duration<double, milli>(t1 - t0).count();
            delete_atom_node_operation_time.push_back(ms);
        }
        double delete_atom_node_total_time = accumulate(
            delete_atom_node_operation_time.begin(), delete_atom_node_operation_time.end(), 0.0);
        global_mutex.lock();
        global_metrics["delete_atom[node]"] =
            Metrics{delete_atom_node_operation_time,
                    delete_atom_node_total_time,
                    delete_atom_node_total_time / iterations_,
                    iterations_ / (delete_atom_node_total_time / 1000.0)};
        global_mutex.unlock();
    }

    void delete_atom_link() {
        vector<double> delete_atom_link_operation_time;
        for (int i = 0; i < iterations_; i++) {
            string suffix = "_t" + to_string(tid_) + "_i" + to_string(i);
            auto node_equivalence = new Node("Symbol", string("EQUIVALENCE_B") + suffix);
            auto node_equivalence_handle = db_->add_node(node_equivalence);
            auto node_e = new Node("Symbol", string("\"NODE_E") + suffix + "\"");
            auto node_e_handle = db_->add_node(node_e);
            auto node_f = new Node("Symbol", string("\"NODE_F") + suffix + "\"");
            auto node_f_handle = db_->add_node(node_f);
            auto link_b =
                new Link("Expression", {node_equivalence_handle, node_e_handle, node_f_handle});
            auto link_b_handle = db_->add_link(link_b);

            auto t0 = Clock::now();
            db_->delete_link(link_b_handle);
            auto t1 = Clock::now();

            double ms = chrono::duration<double, milli>(t1 - t0).count();
            delete_atom_link_operation_time.push_back(ms);
        }
        double delete_atom_link_total_time = accumulate(
            delete_atom_link_operation_time.begin(), delete_atom_link_operation_time.end(), 0.0);
        global_mutex.lock();
        global_metrics["delete_atom[link]"] =
            Metrics{delete_atom_link_operation_time,
                    delete_atom_link_total_time,
                    delete_atom_link_total_time / iterations_,
                    iterations_ / (delete_atom_link_total_time / 1000.0)};
        global_mutex.unlock();
    }

   private:
    int tid_;
    shared_ptr<AtomDB> db_;
    int iterations_;
};

class DeleteAtoms {
   public:
    DeleteAtoms(int tid, shared_ptr<AtomDB> db, int iterations)
        : tid_(tid), db_(db), iterations_(iterations) {}

    void delete_nodes() {
        for (int i = 0; i < iterations_; ++i) {
            // Delete nodes
        }
    }

    void delete_links() {
        for (int i = 0; i < iterations_; ++i) {
            // Delete links
        }
    }

    void delete_atoms_node() {
        for (int i = 0; i < iterations_; ++i) {
            // Delete atom nodes
        }
    }

    void delete_atoms_link() {
        for (int i = 0; i < iterations_; ++i) {
            // Delete atom links
        }
    }

   private:
    int tid_;
    shared_ptr<AtomDB> db_;
    int iterations_;
};

void setup(bool cache_enable) {
    setenv("DAS_REDIS_HOSTNAME", "localhost", 1);
    setenv("DAS_REDIS_PORT", "29000", 1);
    setenv("DAS_USE_REDIS_CLUSTER", "false", 1);
    setenv("DAS_MONGODB_HOSTNAME", "localhost", 1);
    setenv("DAS_MONGODB_PORT", "28000", 1);
    setenv("DAS_MONGODB_USERNAME", "dbadmin", 1);
    setenv("DAS_MONGODB_PASSWORD", "dassecret", 1);
    setenv("DAS_MORK_HOSTNAME", "localhost", 1);
    setenv("DAS_MORK_PORT", "8000", 1);
    if (cache_enable) {
        setenv("DAS_DISABLE_ATOMDB_CACHE", "false", 1);
    } else {
        setenv("DAS_DISABLE_ATOMDB_CACHE", "true", 1);
    }
    AtomDBCacheSingleton::init();
    RedisMongoDB::initialize_statics();
}

shared_ptr<AtomDB> factory_create_atomdb(string type) {
    if (type == "redis_mongo") {
        return make_shared<RedisMongoDB>();
    } else if (type == "mork") {
        return make_shared<MorkDB>();
    } else {
        Utils::error("Unknown AtomDB type: " + type);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0]
             << " <atomdb_type> <action> <cache> <num_concurrency> <num_iterations> <timestamp>" << endl;
        exit(1);
    }

    string atomdb_type = argv[1];
    string action = argv[2];
    string method = argv[3];
    bool cache_enable = ((string(argv[4]) == "enabled") ? true : false);
    int concurrency = stoi(argv[5]);
    int iterations = stoi(argv[6]);
    string timestamp = argv[7];

    // if (iterations < 2) {
    //     Utils::error("Iterations must be greater than 1. Current value: " + to_string(iterations));
    // }

    // if (iterations % 2 != 0) {
    //     Utils::error("Iterations must be even. Current value: " + to_string(iterations));
    // }

    setup(cache_enable);
    auto atomdb = factory_create_atomdb(atomdb_type);

    auto worker = [&](int tid, shared_ptr<AtomDB> atomdb) {
        if (action == "AddAtom") {
            AddAtom benchmark_add_atom(tid, atomdb, iterations);
            if (method == "add_node") {
                benchmark_add_atom.add_node();
            } else if (method == "add_link") {
                benchmark_add_atom.add_link();
            } else if (method == "add_atom_node") {
                benchmark_add_atom.add_atom_node();
            } else if (method == "add_atom_link") {
                benchmark_add_atom.add_atom_link();
            } else {
                Utils::error(
                    "Invalid method. Choose either 'add_node', 'add_link', 'add_atom_node' or "
                    "'add_atom_link'");
            }
        } else if (action == "AddAtoms") {
            AddAtoms benchmark_add_atoms(tid, atomdb, iterations);
            if (method == "add_nodes") {
                benchmark_add_atoms.add_nodes();
            } else if (method == "add_links") {
                benchmark_add_atoms.add_links();
            } else if (method == "add_atoms_node") {
                benchmark_add_atoms.add_atoms_node();
            } else if (method == "add_atoms_link") {
                benchmark_add_atoms.add_atoms_link();
            } else {
                Utils::error(
                    "Invalid method. Choose either 'add_nodes', 'add_links', 'add_atoms_node' or "
                    "'add_atoms_link'");
            }
        } else if (action == "GetAtom") {
            GetAtom benchmark_get_atom(tid, atomdb, iterations);
            if (method == "get_node_document") {
                benchmark_get_atom.get_node_document();
            } else if (method == "get_link_document") {
                benchmark_get_atom.get_link_document();
            } else if (method == "get_atom_document_node") {
                benchmark_get_atom.get_atom_document_node();
            } else if (method == "get_atom_document_link") {
                benchmark_get_atom.get_atom_document_link();
            } else if (method == "get_atom_node") {
                benchmark_get_atom.get_atom_node();
            } else if (method == "get_atom_link") {
                benchmark_get_atom.get_atom_link();
            } else {
                Utils::error(
                    "Invalid method. Choose either 'get_node_document', 'get_link_document', "
                    "'get_atom_document_node', "
                    "'get_atom_document_link', 'get_atom_node' or 'get_atom_link'");
            }
        } else if (action == "GetAtoms") {
            GetAtoms benchmark_get_atoms(tid, atomdb, iterations);
            if (method == "get_node_documents") {
                benchmark_get_atoms.get_node_documents();
            } else if (method == "get_link_documents") {
                benchmark_get_atoms.get_link_documents();
            } else if (method == "get_atom_documents_node") {
                benchmark_get_atoms.get_atom_documents_node();
            } else if (method == "get_atom_documents_link") {
                benchmark_get_atoms.get_atom_documents_link();
            } else if (method == "query_for_pattern") {
                benchmark_get_atoms.query_for_pattern();
            } else if (method == "query_for_targets") {
                benchmark_get_atoms.query_for_targets();
            } else {
                Utils::error(
                    "Invalid method. Choose either 'get_node_documents', 'get_link_documents', "
                    "'get_atom_documents_node', 'get_atom_documents_link', 'query_for_pattern' or "
                    "'query_for_targets'");
            }
        } else if (action == "DeleteAtom") {
            DeleteAtom benchmark_delete_atom(tid, atomdb, iterations);
            if (method == "delete_node") {
                benchmark_delete_atom.delete_node();
            } else if (method == "delete_link") {
                benchmark_delete_atom.delete_link();
            } else if (method == "delete_atom_node") {
                benchmark_delete_atom.delete_atom_node();
            } else if (method == "delete_atom_link") {
                benchmark_delete_atom.delete_atom_link();
            } else {
                Utils::error(
                    "Invalid method. Choose either 'delete_node', 'delete_link', "
                    "'delete_atom_node' or 'delete_atom_link'");
            }
        } else if (action == "DeleteAtoms") {
            DeleteAtoms benchmark_delete_atoms(tid, atomdb, iterations);
            if (method == "delete_nodes") {
                benchmark_delete_atoms.delete_nodes();
            } else if (method == "delete_links") {
                benchmark_delete_atoms.delete_links();
            } else if (method == "delete_atoms_node") {
                benchmark_delete_atoms.delete_atoms_node();
            } else if (method == "delete_atoms_link") {
                benchmark_delete_atoms.delete_atoms_link();
            } else {
                Utils::error(
                    "Invalid method. Choose either 'delete_nodes', 'delete_links', "
                    "'delete_atoms_node' or 'delete_atoms_link'");
            }
        } else {
            Utils::error(
                "Invalid action. Choose either 'AddAtom' or 'AddAtoms' or 'GetAtom' or 'GetAtoms' "
                "or 'DeleteAtom' or 'DeleteAtoms'");
        }
    };

    vector<thread> threads;

    for (int t = 0; t < concurrency; t++) {
        threads.emplace_back(worker, t, atomdb);
    }
    for (auto& th : threads) {
        th.join();
    }

    string base_directory = "/tmp/atomdb_benchmark/" + timestamp;

    int batch_size = 1;
    if (action == "AddAtoms" || action == "DeleteAtoms" || action == "GetAtoms") {
        batch_size = BATCH_SIZE;
    }

    create_report(get_type_name(*atomdb), action, method, global_metrics, base_directory, batch_size);

    return 0;
}