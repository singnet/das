/***
 * @file LinkCreationRequestProxy.h
 */

#pragma once

#include "BaseProxy.h"
#include "Message.h"

using namespace std;
using namespace agents;
using namespace distributed_algorithm_node;

namespace link_creation_agent {
/**
 * Proxy which allows communication between the caller of the LINK_CREATION and
 * the bus element actually executing it.
 *
 * The caller can use this object in order to create links and to abort the link creation
 * process before it finished.
 *
 */
class LinkCreationRequestProxy : public BaseProxy {
   public:
    // ---------------------------------------------------------------------------------------------

    /** Maximum number of answers to return for each query */
    static string MAX_ANSWERS;
    /** Number of times to repeat the link creation request, queries and creates links N times */
    static string REPEAT_COUNT;
    /** Context for the request */
    static string CONTEXT;
    /** Whether to update the attention broker when running queries */
    static string ATTENTION_UPDATE_FLAG;
    /** Importance flag for the request, return only answers that have importance greater than zero */
    static string POSITIVE_IMPORTANCE_FLAG;
    /** Query interval for the request, controls how many time to wait between queries and link
     * creations*/
    static string QUERY_INTERVAL;
    /** Query timeout for the request, controls how long to wait for a query to complete */
    static string QUERY_TIMEOUT;

    static string USE_METTA_AS_QUERY_TOKENS;


    /**
     * Empty constructor typically used on server side.
     */
    LinkCreationRequestProxy();

    /**
     * Basic constructor typically used on client side.
     *
     * @param context Context for the link creation request
     */
    LinkCreationRequestProxy(const vector<string>& tokens);

    /**
     * Destructor.
     */
    virtual ~LinkCreationRequestProxy();

    /**
     * Pack the command line arguments into the args vector.
     *
     */
    virtual void pack_command_line_args();

    /**
     * Set default parameters for the link creation request.
     *
     * This method initializes the default parameters for the request.
     */
    void set_default_parameters();

    /**
     * Set a parameter for the link creation request.
     *
     * @param key The key of the parameter to set.
     * @param value The value of the parameter to set.
     */
    void set_parameter(const string& key, const PropertyValue& value);

   private:
    mutex api_mutex;
};
}  // namespace link_creation_agent
