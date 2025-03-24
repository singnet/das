#include "LeadershipBroker.h"
#include "Utils.h"

using namespace distributed_algorithm_node;

// -------------------------------------------------------------------------------------------------
// Constructors and destructors

LeadershipBroker::LeadershipBroker() {
    this->network_leader_id = "";
}

LeadershipBroker::~LeadershipBroker() {
}

shared_ptr<LeadershipBroker> LeadershipBroker::factory(LeadershipBrokerType instance_type) {
    switch (instance_type) {
        case LeadershipBrokerType::SINGLE_MASTER_SERVER : {
            return shared_ptr<LeadershipBroker>(new SingleMasterServer());
        }
        case LeadershipBrokerType::TRUSTED_BUS_PEER : {
            return shared_ptr<LeadershipBroker>(new TrustedBusPeer());
        }
        default: {
            Utils::error("Invalid LeadershipBrokerType: " + to_string((int) instance_type));
            return shared_ptr<LeadershipBroker>{};
        }
    }
}

SingleMasterServer::SingleMasterServer() {
}

SingleMasterServer::~SingleMasterServer() {
}
  
TrustedBusPeer::TrustedBusPeer() {
}

TrustedBusPeer::~TrustedBusPeer() {
}
  
// -------------------------------------------------------------------------------------------------
// Public superclass API

string LeadershipBroker::leader_id() {
    return this->network_leader_id;
}

void LeadershipBroker::set_leader_id(const string &leader_id) {
    this->network_leader_id = leader_id;
}

bool LeadershipBroker::has_leader() {
    return (this->network_leader_id != "");
}

void LeadershipBroker::set_message_broker(shared_ptr<MessageBroker> message_broker) {
    this->message_broker = message_broker;
}
  
// -------------------------------------------------------------------------------------------------
// Concrete implementation of abstract LeadershipBroker API

void SingleMasterServer::start_leader_election(const string &my_vote) {
    this->set_leader_id(my_vote);
}

void TrustedBusPeer::start_leader_election(const string &my_vote) {
    this->set_leader_id(my_vote);
}
