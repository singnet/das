#pragma once

#include <mutex>
#include "BusCommandProxy.h"
#include "QueryAnswer.h"
#include "SharedQueue.h"
#include "Message.h"

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
class PatternMatchingQueryProxy : public BusCommandProxy {

public:

    // ---------------------------------------------------------------------------------------------
    // Constructors, destructors and static state

    // Commands allowed at the proxy level (caller <--> processor)
    static string ABORT;         // Abort current query
    static string ANSWER_BUNDLE; // Delivery of a bundle with QueryAnswer objects
    static string COUNT;         // Delivery of the final result of a count_only query
    static string FINISHED;      // Notification that all query results have alkready been delivered

    /**
     * Empty constructor typically used on server side.
     */
    PatternMatchingQueryProxy();

    /**
     * Basic constructor typically used on client side.
     *
     * @param tokens Query tokens.
     * @param context AttentionBroker context
     * @param update_attention_broker Flag to trigger AttentionBroker update based on this query
     * results
     * @param count_only Flag to indicate that this query is supposed to count the results and not
     * actually provide the query answers (i.e. no QueryAnswer is sent from the command executor and
     * the caller of the query).
     */
    PatternMatchingQueryProxy(
        const vector<string>& tokens,
        const string& context = "",
        bool update_attention_broker = false,
        bool count_only = false);

    /**
     * Destructor.
     */
    virtual ~PatternMatchingQueryProxy();

    // ---------------------------------------------------------------------------------------------
    // Client-side API

    /**
     * Returns true iff all QueryAnswer objects have been delivered AND iterated.
     *
     * @return true iff all QueryAnswer objects have been delivered AND iterated.
     */
    bool finished();

    /**
     * Pops and returns the next QueryAnswer object or NULL if none is available.
     *
     * NOTE: a NULL return doesn't mean that the query is finished; it only means that there's no
     * QueryAnswer ready to be iterated.
     *
     * @return The next QueryAnswer object or NULL if none is available.
     */
    shared_ptr<QueryAnswer> pop();

    /**
     * Returns the number of QueryAnswers delivered so far or the total number of answers in a
     * count_only query.
     *
     * NOTE: the count (when used in a regular i.e. count_only=false) is for delivered answers,
     * not iterated ones.
     *
     * @return The number of QueryAnswers delivered so far or the total number of answers in a
     * count_only query.
     */
    unsigned int get_count();

    /**
     * Abort a query.
     *
     * When the caller calls this method, a message is sent to the query processor to abort
     * the search for QueryAnswers.
     */
    void abort();

    // ---------------------------------------------------------------------------------------------
    // Server-side API

    /**
     * Returns true iff a request to abort has been issued by the caller.
     *
     * @return true iff a request to abort has been issued by the caller.
     */
    bool is_aborting();

    /**
     * Getter for context
     *
     * @return Context
     */
    const string& get_context();

    /**
     * Setter for context
     *
     * @param context Context
     */
    void set_context(const string& context);

    /**
     * Getter for attention_update_flag
     *
     * attention_update_flag tells the processor whether AttentionBroker is supposed to be updated
     * when the query is being processed.
     *
     * @return attention_update_flag
     */
    bool get_attention_update_flag();

    /**
     * Setter for attention_update_flag
     *
     * attention_update_flag tells the processor whether AttentionBroker is supposed to be updated
     * when the query is being processed.
     *
     * @param flag Flag
     */
    void set_attention_update_flag(bool flag);

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
     * Getter for query_tokens
     *
     * @return query_tokens
     */
    const vector<string>& get_query_tokens();

    // ---------------------------------------------------------------------------------------------
    // Virtual superclass API from_remote_peer() and the piggyback methods called by it

    /**
     * Receive a command and its arguments passed by the remote peer.
     *
     * Concrete subclasses of BusCommandProxy need to implement this method.
     *
     * @param command RPC command
     * @param args RPC command's arguments
     */
    void from_remote_peer(const string& command, const vector<string>& args) override;

    /**
     * Piggyback method called by ABORT command
     *
     * @param args Command arguments (empty for ABORT command)
     */
    void abort(const vector<string>& args);

    /**
     * Piggyback method called by ANSWER_BUNDLE command
     *
     * @param args Command arguments (tokenized QueryAnswer objects)
     */
    void answer_bundle(const vector<string>& args);

    /**
     * Piggyback method called by COUNT command
     *
     * @param args Command arguments (one argument with the total number of answers)
     */
    void count_answer(const vector<string>& args);

    /**
     * Piggyback method called by FINISHED command
     *
     * @param args Command arguments (empty for FINISHED command)
     */
    void query_answers_finished(const vector<string>& args);

    vector<string> query_tokens;

private:

    mutex api_mutex;
    bool abort_flag;
    SharedQueue answer_queue;
    unsigned int answer_count;
    bool answer_flow_finished;
    string context;
    bool update_attention_broker;
    bool count_flag;

    void init();
};

} // namespace query_engine
