#include <string>
#include "HebbianNetworkUpdater.h"
#include "HebbianNetwork.h"
#include "Utils.h"

using namespace attention_broker_server;
using namespace commons;

HebbianNetworkUpdater::HebbianNetworkUpdater() {
}

// --------------------------------------------------------------------------------
// Public methods

HebbianNetworkUpdater::~HebbianNetworkUpdater() {
}

HebbianNetworkUpdater *HebbianNetworkUpdater::factory(HebbianNetworkUpdaterType instance_type) {
    switch (instance_type) {
        case HebbianNetworkUpdaterType:: EXACT_COUNT: {
            return new ExactCountHebbianUpdater();
        }
        default: {
            Utils::error("Invalid HebbianNetworkUpdaterType: " + to_string((int) instance_type));
            return NULL; // to avoid warnings
        }
    }

}

ExactCountHebbianUpdater::ExactCountHebbianUpdater() {
}

ExactCountHebbianUpdater::~ExactCountHebbianUpdater() {
}

void ExactCountHebbianUpdater::correlation(const dasproto::HandleList *request) {
    HebbianNetwork *network = (HebbianNetwork *) request->hebbian_network();
    if (network != NULL) {
        for (const string &s: ((dasproto::HandleList *) request)->list()) {
            network->add_node(s);
        }
        HebbianNetwork::Node *node1;
        HebbianNetwork::Node *node2;
        for (const string &s1: ((dasproto::HandleList *) request)->list()) {
            node1 = network->lookup_node(s1);
            for (const string &s2: ((dasproto::HandleList *) request)->list()) {
                if (s1.compare(s2) < 0) {
                    node2 = network->lookup_node(s2);
                    network->add_symmetric_edge(s1, s2, node1, node2);
                }
            }
        }
    }
}
