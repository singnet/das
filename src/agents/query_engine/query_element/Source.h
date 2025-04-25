#pragma once

#include "QueryAnswer.h"
#include "QueryElement.h"

using namespace std;

namespace query_element {

/**
 * Superclass for elements that represent leaves in the query tree of QueryElement.
 *
 * Source adds the required DistributedAlgorithmNode (actually a specialized version of it
 * named QueryNode) and exposes a public API to interact with it transparently. Basically,
 * a client version of QueryNode (i.e. a ClientQueryNode) is setup to communicate with
 * a remote ServerQueryNode which is located in the QueryElement just above in the query tree.
 */
class Source : public QueryElement {
   public:
    /**
     * Sources tipically need to communicate with the AttentionBroker in order to sort links
     * by importance. AttentionBroker is supposed to be running in the same machine as all
     * Source elements so only a port number is required. Here we provide a default value
     * in the case none is passed in constructor.
     */
    static string DEFAULT_ATTENTION_BROKER_PORT;

    /**
     * Constructor which also sets a value for AttentionBroker address
     */
    Source(const string& attention_broker_address);

    /**
     * Basic empty constructor.
     */
    Source();

    /**
     * Destructor.
     */
    virtual ~Source();

    /**
     * Helper to get AttentionBroker address from env vars or default values.
     */
    static string get_attention_broker_address();

    // --------------------------------------------------------------------------------------------
    // QueryElement API

    /**
     * Setup a ClientQueryNode to communicate with the upper QueryElement.
     */
    virtual void setup_buffers();

    /**
     * Gracefully shuts down threads or any other resources being used in communication.
     */
    virtual void stop();


   protected:
    string attention_broker_address;
    shared_ptr<QueryNode> output_buffer;
    shared_ptr<QueryElement> subsequent;
};

}  // namespace query_element
