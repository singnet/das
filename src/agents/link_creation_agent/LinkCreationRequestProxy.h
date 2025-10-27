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

    static const string MAX_RESULTS;
    static const string REPEAT_COUNT;
    static const string INFINITE_REQUEST;
    static const string CONTEXT;
    static const string UPDATE_ATTENTION_BROKER;
    static const string IMPORTANCE_FLAG;
    static const string QUERY_INTERVAL;
    static const string QUERY_TIMEOUT;

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
