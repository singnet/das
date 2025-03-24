#ifndef _DISTRIBUTED_ALGORITHM_NODE_LEADERSHIPBROKER_H
#define _DISTRIBUTED_ALGORITHM_NODE_LEADERSHIPBROKER_H

#include "MessageBroker.h"

using namespace std;

namespace distributed_algorithm_node {

enum class LeadershipBrokerType {
    SINGLE_MASTER_SERVER,
    TRUSTED_BUS_PEER
};


// -------------------------------------------------------------------------------------------------
// Abstract superclass

/**
 * Implements the algorithm for leader election.
 *
 * This is the abstract class defining the API used by DistributedAlgorithmNodes to deal with leader election.
 * Users of the DistributedAlgorithmNode module aren't supposed to interact with LeadershipBroker directly.
 */
class LeadershipBroker {

public:

    /**
     * Factory method for concrete subclasses.
     *
     * @param instance_type Defines which subclass should be used to instantiate the
     * LeadershipBroker.
     * @return An instance of the selected LeadershipBroker subclass.
     */
    static shared_ptr<LeadershipBroker> factory(LeadershipBrokerType instance_type);

    /**
     * Basic empty constructor
     */
    LeadershipBroker();

    /**
     * Destructor.
     */
    virtual ~LeadershipBroker();

    /**
     * Sets MessageBroker to be used to communicate with peers.
     *
     * @param message_broker The MessageBroker to be used to ciommunicate with peers.
     */
    void set_message_broker(shared_ptr<MessageBroker> message_broker);

    /**
     * Returns the leader node ID.
     *
     * @return The leader node ID.
     */
    string leader_id();

    /**
     * Sets the leader node ID.
     *
     * @param leader_id The leader node ID.
     */
    void set_leader_id(const string &leader_id);

    /**
     * Return true iff a leader has been defined.
     */
    bool has_leader();

    // ----------------------------------------------------------------
    // Public abstract API

    /**
     * Starts an election to define the leader of the network.
     *
     * @param my_vote The vote casted by the hosting node to tghe leadership election.
     */
    virtual void start_leader_election(const string &my_vote) = 0;


private:

    shared_ptr<MessageBroker> message_broker;
    string network_leader_id;
};

// -------------------------------------------------------------------------------------------------
// Concrete subclasses

/**
 * Concrete implementation of a leadership selection algorithm based in a topology of one server
 * connected to N clients, where clients only know about the server which is always the leader of
 * the network.
 */
class SingleMasterServer : public LeadershipBroker {

public:

    /**
     * Basic constructor
     */
    SingleMasterServer();

    /**
     * Destructor
     */
    ~SingleMasterServer();


    // ----------------------------------------------------------------
    // Public LeadershipBroker abstract API

    void start_leader_election(const string &my_vote);
};

/**
 * Concrete implementation of a leadership selection algorithm in a service BUS where there are
 * no leaders, only known trusted peers. Each bus node has either 1 trusted peer or ZERO, when
 * it was the first node to join the bus.
 */
class TrustedBusPeer : public LeadershipBroker {

public:

    /**
     * Basic constructor
     */
    TrustedBusPeer();

    /**
     * Destructor
     */
    ~TrustedBusPeer();


    // ----------------------------------------------------------------
    // Public LeadershipBroker abstract API

    void start_leader_election(const string &my_vote);
};

} // namespace distributed_algorithm_node

#endif // _DISTRIBUTED_ALGORITHM_NODE_LEADERSHIPBROKER_H

