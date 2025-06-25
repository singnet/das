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
 * Proxy which allows communication between the caller of the LINK_CREATE_REQUEST and
 * the bus element actually executing it.
 *
 * The caller can use this object in order to create links and to abort the link creation
 * process before it finished.
 *
 */
class LinkCreationRequestProxy : public BaseProxy {
   public:
    // ---------------------------------------------------------------------------------------------
    struct Commands {
        static const string ABORT;  // Abort command
    };

    struct Parameters {
        static const string QUERY_INTERVAL;
        static const string QUERY_TIMEOUT;
    };

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

    virtual void pack_command_line_args();

    void set_default_parameters();

    void set_parameter(const string& key, const PropertyValue& value);

   private:
    void init();

    mutex api_mutex;
    bool abort_flag;
};
}  // namespace link_creation_agent