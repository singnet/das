#include "expression_hasher.h"
#include "StimulusSpreader.h"
#include "HebbianNetwork.h"
#include "AttentionBrokerServer.h"
#include "Utils.h"
#include <string>
#include <forward_list>

using namespace attention_broker_server;

// --------------------------------------------------------------------------------
// Public constructors and destructors

StimulusSpreader::~StimulusSpreader() {
}

StimulusSpreader::StimulusSpreader() {
}

StimulusSpreader *StimulusSpreader::factory(StimulusSpreaderType instance_type) {
    switch (instance_type) {
        case StimulusSpreaderType::TOKEN : {
            return new TokenSpreader();
        }
        default: {
            Utils::error("Invalid StimulusSpreaderType: " + to_string((int) instance_type));
            return NULL; // to avoid warnings
        }
    }

}

TokenSpreader:: TokenSpreader() {
}

TokenSpreader:: ~TokenSpreader() {
}

// ------------------------------------------------
// "visit" functions used to traverse network

typedef TokenSpreader::StimuliData DATA;

static bool collect_rent(HandleTrie::TrieNode *node, void *data) {
    ImportanceType rent = ((DATA *) data)->rent_rate * \
                          ((HebbianNetwork::Node *) node->value)->importance;
    ((DATA *) data)->total_rent += rent;
    ImportanceType wages = 0.0;
    ((DATA *) data)->importance_changes->insert(
        node->suffix,
        new TokenSpreader::ImportanceChanges(rent, wages));
    return false;
}

static bool consolidate_rent_and_wages(HandleTrie::TrieNode *node, void *data) {

    HebbianNetwork::Node *value = (HebbianNetwork::Node *) node->value;

    TokenSpreader::ImportanceChanges *changes =\
        (TokenSpreader::ImportanceChanges *) ((DATA *) data)->importance_changes->lookup(node->suffix);
    value->importance -= changes->rent;
    value->importance += changes->wages;

    // Compute amount to be spread
    ImportanceType arity_ratio = (double) value->arity / ((DATA *) data)->largest_arity;
    ImportanceType spreading_rate = ((DATA *) data)->spreading_rate_lowerbound + \
                                    (((DATA *) data)->spreading_rate_range_size * \
                                     arity_ratio);
    ImportanceType to_spread = value->importance * spreading_rate;
    value->importance -= to_spread;
    value->stimuli_to_spread = to_spread;
    return false;
}

static bool sum_weights(HandleTrie::TrieNode *node, void *data) {
    HebbianNetwork::Edge *edge = (HebbianNetwork::Edge *) node->value;
    double w = (double) edge->count / edge->node1->count;
    ((DATA *) data)->sum_weights += w;
    return false;
}

static bool deliver_stimulus(HandleTrie::TrieNode *node, void *data) {
    HebbianNetwork::Edge *edge = (HebbianNetwork::Edge *) node->value;
    double w = (double) edge->count / edge->node1->count;
    ImportanceType stimulus = (w / ((DATA *) data)->sum_weights) * ((DATA *) data)->to_spread;
    edge->node2->importance += stimulus;
    return false;
}

static bool consolidate_stimulus(HandleTrie::TrieNode *node, void *data) {
    HebbianNetwork::Node *value = (HebbianNetwork::Node *) node->value;
    ((DATA *) data)->to_spread = value->stimuli_to_spread;
    ((DATA *) data)->sum_weights = 0.0;
    value->neighbors->traverse(true, &sum_weights, data);
    value->neighbors->traverse(true, &deliver_stimulus, data);
    value->stimuli_to_spread = 0.0;
    return false;
}

// ------------------------------------------------
// Public methods

void TokenSpreader::distribute_wages(
    const dasproto::HandleCount *handle_count,
    ImportanceType &total_to_spread,
    DATA *data) {

    auto iterator = handle_count->map().find("SUM");
    if (iterator == handle_count->map().end()) {
        Utils::error("Missing 'SUM' key in HandleCount request");
    }
    unsigned int total_wages = iterator->second;
    for (auto pair: handle_count->map()) {
        if (pair.first != "SUM") {
            double normalized_amount = (((double) pair.second) * total_to_spread) / total_wages;
            data->importance_changes->insert(pair.first, new TokenSpreader::ImportanceChanges(0.0, normalized_amount));
        }
    }
}

void TokenSpreader::spread_stimuli(const dasproto::HandleCount *request) {

    HebbianNetwork *network = (HebbianNetwork *) request->hebbian_network();
    if (network == NULL) {
        return;
    }

    DATA data;
    data.importance_changes = new HandleTrie(HANDLE_HASH_SIZE - 1);
    data.rent_rate = AttentionBrokerServer::RENT_RATE;
    data.spreading_rate_lowerbound = AttentionBrokerServer::SPREADING_RATE_LOWERBOUND;
    data.spreading_rate_range_size = \
        AttentionBrokerServer::SPREADING_RATE_UPPERBOUND - AttentionBrokerServer::SPREADING_RATE_LOWERBOUND;
    data.largest_arity = network->largest_arity;
    data.total_rent = 0.0;

    // Collect rent
    network->visit_nodes(true, &collect_rent, (void *) &data);

    // Distribute wages
    ImportanceType total_to_spread = network->alienate_tokens();
    total_to_spread += data.total_rent;
    distribute_wages(request, total_to_spread, &data);

    // Consolidate changes
    network->visit_nodes(true, &consolidate_rent_and_wages, (void *) &data);

    // Spread activation (1 cycle)
    network->visit_nodes(true, &consolidate_stimulus, &data);
}
