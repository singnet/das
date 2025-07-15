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

    // Query command's optional parameters
    static string UNIQUE_ASSIGNMENT_FLAG;  // When true, query operators (e.g. And, Or) don't output
                                           // more than one QueryAnswer with the same variable
                                           // assignment.
    static string ATTENTION_UPDATE_FLAG;   // When true, queries issued to the pattern matcher will
                                           // trigger attention update automatically.

    static string MAX_BUNDLE_SIZE;  // Max number of answers buffered before flushing them to the peer
                                    // client proxy.

    static string MAX_ANSWERS; // Limits the number of returned answers

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

    /**
     * Write a tokenized representation of this proxy in the passed `output` vector.
     *
     * @param output Vector where the tokens will be put.
     */
    virtual void tokenize(vector<string>& output);

    // ---------------------------------------------------------------------------------------------
    // Server-side API

    /**
     * Extrtact the tokens from the begining of the passed tokens vector (AND ERASE THEM) in order
     * to build this proxy.
     *
     * @param tokens Tokens vector (CHANGED BY SIDE-EFFECT)
     */
    virtual void untokenize(vector<string>& tokens);

    /**
     * Push an answer to the proxy. This answer will end in the peer proxy but it may not happen
     * immediatelly because of the answer buffering algorithm.
     *
     * @param answer Answer to push.
     */
    virtual void push(shared_ptr<QueryAnswer> answer);

    /**
     * Send current answer bundle to remote client peer.
     */
    void flush_answer_bundle();

    /**
     * Called by processor to indicate that the processing of the current query is finished.
     */
    void query_processing_finished();

    /**
     * Getter for context
     *
     * @return Context
     */
    const string& get_context();

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
    // Virtual superclass API and the piggyback methods called by it

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

    virtual void pack_command_line_args() = 0;

   private:
    void init();
    mutex api_mutex;
    SharedQueue answer_queue;
    unsigned int answer_count;
    string context;
    vector<string> query_tokens;
    vector<string> answer_bundle_vector;
};

}  // namespace agents
