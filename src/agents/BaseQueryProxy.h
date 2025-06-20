#pragma once

#include <mutex>

#include "BaseProxy.h"
#include "Message.h"
#include "QueryAnswer.h"
#include "SharedQueue.h"

using namespace std;
using namespace service_bus;
using namespace query_engine;
using namespace agents;

namespace agents {

class BaseQueryProxy : public BaseProxy {
   protected:  // This is an abstract class
    /**
     * Empty constructor typically used on server side.
     */
    BaseQueryProxy();

    /**
     * Basic constructor typically used on client side.
     *
     * @param tokens Query tokens.
     * @param context AttentionBroker context
     */
    BaseQueryProxy(const vector<string>& tokens, const string& context);

   public:
    // ---------------------------------------------------------------------------------------------
    // Constructors, destructors and static state

    // Commands allowed at the proxy level (caller <--> processor)
    static string ANSWER_BUNDLE;  // Delivery of a bundle with QueryAnswer objects
    static string ABORT;          // Abort current query
    static string FINISHED;       // Notification that all query results have alkready been delivered

    /**
     * Destructor.
     */
    virtual ~BaseQueryProxy();

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

    /**
     * Returns the number of answers delivered so far.
     *
     * NOTE: the count is for delivered answers not iterated ones.
     *
     * @return The number of answers delivered so far.
     */
    unsigned int get_count();

    /**
     * Sets answers count.
     */
    void set_count(unsigned int count);

    // ---------------------------------------------------------------------------------------------
    // Server-side API

    /**
     * Push an answer to the proxy. This answer will end in the peer proxy but it may not happen
     * immediatelly because of the answer bundle algorithm.
     *
     * @param answer Answer to push.
     */
    void push(shared_ptr<QueryAnswer> answer);

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
     * Getter for query_tokens
     *
     * @return query_tokens
     */
    const vector<string>& get_query_tokens();

    /**
     * Returns a string representation with all command parameter values.
     *
     * @return a string representation with all command parameter values.
     */
    virtual string to_string();

    // ---------------------------------------------------------------------------------------------
    // Query parameters getters and setters

    /**
     * Getter for unique_assignment_flag
     *
     * unique_assignment_flag prevents duplicated variable assignment in Operators' output
     *
     * @return unique_assignment_flag
     */
    bool get_unique_assignment_flag();

    /**
     * Setter for unique_assignment_flag
     *
     * unique_assignment_flag prevents duplicated variable assignment in Operators' output
     *
     * @param flag Flag
     */
    void set_unique_assignment_flag(bool flag);

    /**
     * Getter for attention_update_flag
     *
     * attention_update_flag Atutomatically trigger attention allocation update on pattern matcher
     * queries.
     *
     * @return attention_update_flag
     */
    bool get_attention_update_flag();

    /**
     * Setter for attention_update_flag
     *
     * attention_update_flag Atutomatically trigger attention allocation update on pattern matcher
     * queries.
     *
     * @param flag Flag
     */
    void set_attention_update_flag(bool flag);

    // ---------------------------------------------------------------------------------------------
    // Virtual superclass API and the piggyback methods called by it

    /**
     * Piggyback method called when raise_error_on_peer() is called in peer's side.
     *
     * error_code == 0 means that NO ERROR CODE has been provided
     */
    void raise_error(const string& error_message, unsigned int error_code = 0);

    /**
     * Receive a command and its arguments passed by the remote peer.
     *
     * Concrete subclasses of BusCommandProxy need to implement this method.
     *
     * @param command RPC command
     * @param args RPC command's arguments
     */
    virtual bool from_remote_peer(const string& command, const vector<string>& args) override;

    /**
     * Piggyback method called by ANSWER_BUNDLE command
     *
     * @param args Command arguments (tokenized QueryAnswer objects)
     */
    void answer_bundle(const vector<string>& args);

    /**
     * Piggyback method called by FINISHED command
     *
     * @param args Command arguments (empty for FINISHED command)
     */
    void query_answers_finished(const vector<string>& args);

    vector<string> query_tokens;
    bool error_flag;
    unsigned int error_code;
    string error_message;

   private:
    void init();
    mutex api_mutex;
    SharedQueue answer_queue;
    unsigned int answer_count;
    string context;

    // ---------------------------------------------------------------------------------------------
    // Query parameters

    /**
     * When true, query operators (e.g. And, Or) don't output more than one
     * QueryAnswer with the same variable assignment.
     * */
    bool unique_assignment_flag;

    /**
     * When true, queries issued to the pattern matcher will trigger attention update automatically.
     * */
    bool attention_update_flag;
};

}  // namespace agents
