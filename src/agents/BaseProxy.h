#pragma once

#include <mutex>

#include "BusCommandProxy.h"
#include "Properties.h"
#include "Message.h"

using namespace std;
using namespace service_bus;

namespace agents {

/**
 * Basic proxy with commom commands.
 */
class BaseProxy : public BusCommandProxy {
   public:
    // Commands allowed at the proxy level (caller <--> processor)
    static string ABORT;     // Abort current command
    static string FINISHED;  // Notification that all results have already been delivered

    BaseProxy();
    virtual ~BaseProxy();

    // ---------------------------------------------------------------------------------------------
    // Client-side API

    /**
     * Returns true iff all QueryAnswer objects have been delivered AND iterated.
     *
     * @return true iff all QueryAnswer objects have been delivered AND iterated.
     */
    virtual bool finished();

    /**
     * Abort a query.
     *
     * When the caller calls this method, a message is sent to the query processor to abort
     * the search for QueryAnswers.
     */
    void abort();

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
     * Returns true iff a request to abort has been issued by the caller.
     *
     * @return true iff a request to abort has been issued by the caller.
     */
    bool is_aborting();

    /**
     * Returns a string representation with all command parameter values.
     *
     * @return a string representation with all command parameter values.
     */
    virtual string to_string();

    // ---------------------------------------------------------------------------------------------
    // Virtual superclass API and the piggyback methods called by it

    /**
     * Piggyback method called when raise_error_on_peer() is called in peer's side.
     *
     * error_code == 0 means that NO ERROR CODE has been provided
     */
    virtual void raise_error(const string& error_message, unsigned int error_code = 0);

    /**
     * Receive a command and its arguments passed by the remote peer.
     *
     * @param command RPC command
     * @param args RPC command's arguments
     */
    virtual bool from_remote_peer(const string& command, const vector<string>& args) override;

    /**
     * Piggyback method called by ABORT command
     *
     * @param args Command arguments (empty for ABORT command)
     */
    void abort(const vector<string>& args);

    /**
     * Piggyback method called by FINISHED command
     *
     * @param args Command arguments (empty for FINISHED command)
     */
    void command_finished(const vector<string>& args);

    Properties parameters;
    bool error_flag;
    unsigned int error_code;
    string error_message;

   private:
    mutex api_mutex;
    bool abort_flag;
    bool command_finished_flag;
};

}  // namespace agents
