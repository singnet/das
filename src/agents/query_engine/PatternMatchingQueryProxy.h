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
     * Pack query arguments into args vector
     */
    void pack_custom_args();

    /**
     * Returns a string representation with all command parameter values.
     *
     * @return a string representation with all command parameter values.
     */
    virtual string to_string();

    // ---------------------------------------------------------------------------------------------
    // Query parameters getters and setters

    /**
     * Getter for count_flag
     *
     * count_flag tells the processor whether the query is count_only
     *
     * @return count_flag
     */
    bool get_count_flag();

    /**
     * Setter for count_flag
     *
     * count_flag tells the processor whether the query is count_only
     *
     * @param flag Flag
     */
    void set_count_flag(bool flag);

    /**
     * Getter for positive_importance_flag
     *
     * positive_importance_flag indicates that only answers whose importance > 0 are to be returned
     *
     * @return positive_importance_flag
     */
    bool get_positive_importance_flag();

    /**
     * Setter for positive_importance_flag
     *
     * positive_importance_flag indicates that only answers whose importance > 0 are to be returned
     *
     * @param flag Flag
     */
    void set_positive_importance_flag(bool flag);

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
    void set_default_query_parameters();

    mutex api_mutex;

    // ---------------------------------------------------------------------------------------------
    // Query parameters

    // Indicates that only answres whose importance > 0 are supposed to be returned
    bool positive_importance_flag;

    // indicates that this query is supposed to count the results and not
    // actually provide the query answers (i.e. no QueryAnswer is sent from the
    // command executor and the caller of the query).
    bool count_flag;
};

}  // namespace query_engine
