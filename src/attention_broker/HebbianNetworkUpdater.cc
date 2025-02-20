#include "HebbianNetworkUpdater.h"

#include <string>

#include "HebbianNetwork.h"
#include "Utils.h"

using namespace attention_broker_server;
using namespace commons;

HebbianNetworkUpdater::HebbianNetworkUpdater() = default;

// --------------------------------------------------------------------------------
// Public methods

HebbianNetworkUpdater::~HebbianNetworkUpdater() = default;

HebbianNetworkUpdater *
HebbianNetworkUpdater::factory(HebbianNetworkUpdaterType instance_type) {
  switch (instance_type) {
  case HebbianNetworkUpdaterType::EXACT_COUNT: {
    return new ExactCountHebbianUpdater();
  }
  default: {
    Utils::error("Invalid HebbianNetworkUpdaterType: " +
                 to_string((int)instance_type));
    return nullptr; // to avoid warnings
  }
  }
}

ExactCountHebbianUpdater::ExactCountHebbianUpdater() = default;

ExactCountHebbianUpdater::~ExactCountHebbianUpdater() = default;

void ExactCountHebbianUpdater::correlation(
    const dasproto::HandleList *request) {
  auto *network = (HebbianNetwork *)request->hebbian_network();
  if (network != nullptr) {
    for (const string &s : ((dasproto::HandleList *)request)->list()) {
      network->add_node(s);
    }
    HebbianNetwork::Node *node1 = nullptr;
    HebbianNetwork::Node *node2 = nullptr;
    for (const string &s1 : ((dasproto::HandleList *)request)->list()) {
      node1 = network->lookup_node(s1);
      for (const string &s2 : ((dasproto::HandleList *)request)->list()) {
        if (s1.compare(s2) < 0) {
          node2 = network->lookup_node(s2);
          network->add_symmetric_edge(s1, s2, node1, node2);
        }
      }
    }
  }
}
