#pragma once

#include <mutex>

#include "BaseQueryProxy.h"
#include "Message.h"
#include "QueryAnswer.h"
#include "SharedQueue.h"

using namespace std;
using namespace service_bus;
using namespace distributed_algorithm_node;

namespace query_engine {

/**
 * Proxy which allows communication between the caller of the PATTERN_MATCHONG_COMMAND and
 * the bus element actually executing it.
 *
 * The caller can use this object in order to iterate through results of the query and to
 * abort the query execution before it finished.
 *
 * On the command processor side, this object is used to retrieve query parameters (e.g.
 * the actual query tokens, flags etc).
 */
class PatternMatchingQueryProxy : public BaseQueryProxy {
   public:
    // ---------------------------------------------------------------------------------------------
    // Constructors, destructors and static state

    // Commands allowed at the proxy level (caller <--> processor)
    static string COUNT;  // Delivery of the final result of a count_only query

    // Query command's optional parameters
    static string POSITIVE_IMPORTANCE_FLAG;   // Indicates that only answers whose importance > 0
                                              // are supposed to be returned

    static string DISREGARD_IMPORTANCE_FLAG;  // When set true, importance values are not fetched from
                                              // Attention Broker, thus the order of query answers
                                              // are not determined by importance.

    static string UNIQUE_VALUE_FLAG;  // When true, QueryAnswers won't be allowed to have the same
                                      // handle assigned to different values. For instance, if a
                                      // variable V1 is assigned to a handle H1, if this parameter
                                      // is true then it's assured that no other variable will be
                                      // assigned with the same value H1. When this parameter is
                                      // false (which is the default value, btw), it's possible
                                      // to have a QueryAnswer with an assignment like this, for
                                      // example: V1: H1, V2: H2, V3, H1.

    static string COUNT_FLAG;  // Indicates that this query is supposed to count the results and not
                               // actually provide the query answers (i.e. no QueryAnswer is sent
                               // from the command executor and the caller of the query).

    /**
     * Empty constructor typically used on server side.
     */
    PatternMatchingQueryProxy();

    /**
     * Basic constructor typically used on client side.
     *
     * @param tokens Query tokens.
     * @param context AttentionBroker context
     */
    PatternMatchingQueryProxy(const vector<string>& tokens, const string& context);

    /**
     * Destructor.
     */
    virtual ~PatternMatchingQueryProxy();

    // ---------------------------------------------------------------------------------------------
    // Client-side API

    /**
     * Pops and returns the next QueryAnswer object or NULL if none is available.
     *
     * NOTE: a NULL return doesn't mean that the query is finished; it only means that there's no
     * QueryAnswer ready to be iterated.
     *
     * @return The next QueryAnswer object or NULL if none is available.
     */
    virtual shared_ptr<QueryAnswer> pop();

    // ---------------------------------------------------------------------------------------------
    // Server-side API

    /**
     * Builds the args vector to be passed in the RPC
     */
    virtual void pack_command_line_args();

    // ---------------------------------------------------------------------------------------------
    // Virtual superclass API and the piggyback methods called by it

    /**
     * Receive a command and its arguments passed by the remote peer.
     *
     * Concrete subclasses of BusCommandProxy need to implement this method.
     *
     * @param command RPC command
     * @param args RPC command's arguments
     */
    bool from_remote_peer(const string& command, const vector<string>& args) override;

    /**
     * Piggyback method called by COUNT command
     *
     * @param args Command arguments (one argument with the total number of answers)
     */
    void count_answer(const vector<string>& args);

   private:
    void init();
    void set_default_parameters();

    mutex api_mutex;
};

}  // namespace query_engine
