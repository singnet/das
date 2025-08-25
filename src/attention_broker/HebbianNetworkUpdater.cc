#include "HebbianNetworkUpdater.h"

#include <string>

#include "Utils.h"

using namespace attention_broker_server;
using namespace commons;

HebbianNetworkUpdater::HebbianNetworkUpdater() {}

// --------------------------------------------------------------------------------
// Public methods

HebbianNetworkUpdater::~HebbianNetworkUpdater() {}

HebbianNetworkUpdater* HebbianNetworkUpdater::factory(HebbianNetworkUpdaterType instance_type) {
    switch (instance_type) {
        case HebbianNetworkUpdaterType::EXACT_COUNT: {
            return new ExactCountHebbianUpdater();
        }
        default: {
            Utils::error("Invalid HebbianNetworkUpdaterType: " + to_string((int) instance_type));
            return NULL;  // to avoid warnings
        }
    }
}

void HebbianNetworkUpdater::determiners(const dasproto::HandleList& sub_request, HebbianNetwork* network) {
    unsigned int num_handles = sub_request.list_size();
    if (network != NULL && (num_handles > 1)) {
        HebbianNetwork::Node* node1 = network->lookup_node(sub_request.list(0));
        if (node1 == NULL) {
            node1 = network->add_node(sub_request.list(0));
            node1->count = 0;
        }
        for (unsigned int i = 1; i < num_handles; i++) {
            HebbianNetwork::Node* node2 = network->lookup_node(sub_request.list(i));
            if (node2 == NULL) {
                node2 = network->add_node(sub_request.list(i));
                node2->count = 0;
            }
            node1->determiners.insert(node2);
        }
    }
}

ExactCountHebbianUpdater::ExactCountHebbianUpdater() {}

ExactCountHebbianUpdater::~ExactCountHebbianUpdater() {}

void ExactCountHebbianUpdater::correlation(const dasproto::HandleList* request) {
    HebbianNetwork* network = (HebbianNetwork*) request->hebbian_network();
    if (network != NULL) {
        for (const string& s : ((dasproto::HandleList*) request)->list()) {
            network->add_node(s);
        }
        HebbianNetwork::Node* node1;
        HebbianNetwork::Node* node2;
        for (const string& s1 : ((dasproto::HandleList*) request)->list()) {
            node1 = network->lookup_node(s1);
            for (const string& s2 : ((dasproto::HandleList*) request)->list()) {
                if (s1.compare(s2) < 0) {
                    node2 = network->lookup_node(s2);
                    network->add_symmetric_edge(s1, s2, node1, node2);
                }
            }
        }
    }
}

