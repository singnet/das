#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "AtomDB.h"
#include "AtomDBSingleton.h"
#include "RedisMongoDB.h"

using namespace std;
using namespace atomdb;

void setup(bool cache_enable) {
  setenv("DAS_REDIS_HOSTNAME", "localhost", 1);
  setenv("DAS_REDIS_PORT", "29000", 1);
  setenv("DAS_USE_REDIS_CLUSTER", "false", 1);
  setenv("DAS_MONGODB_HOSTNAME", "localhost", 1);
  setenv("DAS_MONGODB_PORT", "28000", 1);
  setenv("DAS_MONGODB_USERNAME", "dbadmin", 1);
  setenv("DAS_MONGODB_PASSWORD", "dassecret", 1);
  if (cache_enable) {
    setenv("DAS_DISABLE_ATOMDB_CACHE", "false", 1);
  } else {
    setenv("DAS_DISABLE_ATOMDB_CACHE", "true", 1);
  }
}

int main(int argc, char** argv) {
  if (argc < 2) {
    cerr << "Usage: " << argv[0]
         << " <action> <cache:enabled|disabled> <num_concurrency> <num_iterations> <metrics>"
         << endl;
    exit(1);
  }

  string action = string(argv[1]);
  bool cache_enable = ((string(argv[2]) == "enabled") ? true : false);
  int concurrency = stoi(argv[3]);
  int iterations = stoi(argv[4]);

  setup(cache_enable);
  
  AtomDBSingleton::init();
  auto atomdb = AtomDBSingleton::get_instance();
  shared_ptr<RedisMongoDB> db = dynamic_pointer_cast<RedisMongoDB>(atomdb);

  // atomic<size_t> counter{0};
  // auto worker = [&](int tid) {
  for (int i = 0; i < iterations; ++i) {
    if (action == "Add Atom") {
      // add noode
      auto equivalence = new Node("Symbol", "EQUIVALENCE3");
      auto equivalence_h = equivalence->handle();
      
      auto node1 = new Node("Symbol", "\"add node xx0000\"");
      auto node1_h = node1->handle();
      auto node_document1 = db->get_atom_document(node1_h);
      if (node_document1 == nullptr) {
          cout << "Adding node1: " << node1_h << endl;
      }
      auto node2 = new Node("Symbol", "\"add node yy0000\"");
      auto node2_h = node2->handle();
      auto node_document2 = db->get_atom_document(node2_h);
      if (node_document2 == nullptr) {
          cout << "Adding node2: " << node2_h << endl;
      }
      
      db->add_node(equivalence);  // add equivalence node
      cout << "Adding node equivalence: " << equivalence_h << endl;
      db->add_node(node1);
      db->add_node(node2);

      // add_link
      auto link = new Link("Expression", {equivalence_h, node1_h, node2_h});
      auto link_handle = link->handle();
      auto link_exists = db->link_exists(link_handle.c_str());
      if (!link_exists) {
          cout << "Adding link: " << link_handle << endl;
      }
      db->add_link(link);

      db->delete_atom(equivalence_h);  // delete equivalence to test if it is added again
      db->delete_atom(node1_h);  // delete node1 to test if it is added again
      db->delete_atom(node2_h);  // delete node2 to test if it is added again
      db->delete_atom(link_handle);  // delete link to test if it is added again

    }       
    else if (action == "Get Atoms") {
      cout << "db.getAtoms()" << endl;
    }
    // ++counter;
  }
  // };

  // vector<thread> threads;
  // auto t_start = chrono::high_resolution_clock::now();
  // for (int t = 0; t < 10; ++t) {
  //   threads.emplace_back(worker, t);
  // }
  // for (auto& th : threads) th.join();
  // auto t_end = chrono::high_resolution_clock::now();

  // double seconds = chrono::duration<double>(t_end - t_start).count();
  // size_t total_ops = counter.load() * 2;
  // cout << "=== Benchmarks Result ===\n";
  // // cout << "DB: " << cfg.db
  // //           << " | Rel: " << cfg.rel
  // //           << " | Cache: " << (cfg.cache_enabled ? "on" : "off") << "\n";
  // // cout << "Threads: " << cfg.concurrency
  // //           << " | Iterations/thread: " << cfg.iterations << "\n";
  // cout << "Total operations: " << total_ops << "\n";
  // cout << "Total time (s): " << seconds << "\n";
  // cout << "Throughput (ops/s): " << (total_ops / seconds) << "\n";
  return 0;
}
