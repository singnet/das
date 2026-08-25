#pragma once

#include <mutex>

#include "BusCommandProxy.h"
#include "Message.h"
#include "Properties.h"

using namespace std;
using namespace service_bus;

namespace agents {

/**
 * Basic proxy with commom commands.
 */
class BaseProxy : public BusCommandProxy {
   public:
    enum ORCHESTRATION_SCHEMA_TYPE { NONE = 0, SYNC_ON_CYCLE_START };

    // BaseProxy optional parameters
    static string ORCHESTRATION_SCHEMA;  // Select orchestration schema for this proxy. Orchestration
                                         // is relevant to processors that work execute commands in
                                         // multiple cycles. The caller may require to synchronize
                                         // the cycles with cycles of other command execution
                                         // (i.e. other proxies) of the same type or not. So
                                         // different orchestration algorithms may apply.

    // Commands allowed at the proxy level (caller <--> processor)
    static string ABORT;              // Abort current command
    static string FINISHED;           // Notification that all results have already been delivered
    static string ALLOW_CYCLE_START;  // Orchestration command to allow the beginning of a new
                                      // cycle in the remote peer.
    static string CYCLE_ENDED;        // Orchestration command to notify remote peer that a cycle
                                      // just ended

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
     * Allows remote proxy to start a new cycle.
     */
    void allow_cycle_start();

    /**
     * Return true iff the remote peer is waiting to start a new cycle.
     */
    bool get_waiting_flag();

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
     * Returns true iff processor has green light to start a new cycle.
     *
     * SIDE-EFFECT: the state flag used tell if a new cycle start is
     *              allowed is reset to FALSE.
     *
     * @return true iff processor has green light to start a new cycle.
     */
    bool is_cycle_start_allowed();

    /**
     * Notifies remote proxy that a cycle just ended.
     */
    void cycle_ended();

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

    /**
     * Piggyback method called by ALLOW_CYCLE_START command
     *
     * @param args Command arguments (empty for ALLOW_CYCLE_START command)
     */
    void allow_cycle_start(const vector<string>& args);

    /**
     * Piggyback method called by CYCLE_ENDED command
     *
     * @param args Command arguments (empty for CYCLE_ENDED command)
     */
    void cycle_ended(const vector<string>& args);

    virtual void pack_command_line_args() = 0;

    Properties parameters;
    bool error_flag;
    unsigned int error_code;
    string error_message;

   protected:
    void set_orchestration_schema(ORCHESTRATION_SCHEMA_TYPE value);

   private:
    mutex api_mutex;
    bool abort_flag;
    bool command_finished_flag;
    ORCHESTRATION_SCHEMA_TYPE orchestration_schema;
    bool cycle_start_allowed_flag;
    bool waiting_log_flag;
    bool waiting_to_start_new_cycle; // disregarded if orchestration_schema is NONE.
};

}  // namespace agents
