/***
 * @file LinkCreationRequestProxy.h
 */

#pragma once

#include "BusCommandProxy.h"
#include "Message.h"

using namespace std;
using namespace service_bus;
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
class LinkCreationRequestProxy : public BusCommandProxy {
   public:
    // ---------------------------------------------------------------------------------------------
    // Constructors, destructors and static state

    // Commands allowed at the proxy level (caller <--> processor)
    static string ABORT;                 // Abort current link creation
    static string LINK_CREATION_FAILED;  // Notification that a link creation failed
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

    // ---------------------------------------------------------------------------------------------
    // Client-side API

    /**
     * Returns true iff all link creation requests have been processed.
     *
     * @return true iff all link creation requests have been processed.
     */
    bool finished();

    /**
     * Abort the link creation process.
     */
    void abort();

    unsigned int error_code;  // Error code if an error occurred
    string error_message;     // Error message if an error occurred

    /**
     * Raise an error in the remote peer.
     *
     * @param error_message Error message to be raised
     * @param error_code Error code to be raised
     */
    virtual void raise_error(const string& error_message, unsigned int error_code = 0) override;

    virtual void pack_custom_args() {
        // No custom args to pack for this proxy
    }

    virtual void pack_command_line_args() override {
        // No command line args to pack for this proxy
    }

   private:
    void init();

    mutex api_mutex;
    bool abort_flag;
};
}  // namespace link_creation_agent