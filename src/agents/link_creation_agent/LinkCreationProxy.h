#pragma once

#include "BaseQueryProxy.h"
#include "LinkCreator.h"

using namespace link_creators;
using namespace std;

namespace link_creation_agent {

/**
 * Proxy which allows communication between the caller of the LINK_CREATION command and
 * the bus element actually executing it.
 *
 * The caller can use this object in order to iterate newly created links or to
 * abort the command execution before it finished.
 *
 * On the command processor side, this object is used to retrieve command parameters (e.g.
 * the actual query tokens, flags etc).
 */
class LinkCreationProxy : public BaseQueryProxy {
   public:
    // ---------------------------------------------------------------------------------------------
    // Constructors, destructors and static state

    // LCA command's optional parameters
    static string MAX_SUCCESSFUL_CREATION_PER_ROUND;
    static string MAX_UNPRODUCTIVE_VISITS_PER_ROUND;
    static string MAX_VISIT_ATTEMPTS_PER_ROUND;
    static string MAX_ROUNDS;
    static string LINK_CREATION_STRENGTH_THRESHOLD;
    static string LINK_CREATION_LOG_FILE_NAME;

    // LOG_NEW_LINKS is an optional parameter but it is not part of the configuration file as it
    // is meant to be used only in tests.
    static string LOG_NEW_LINKS;

    LinkCreationProxy();

    LinkCreationProxy(const vector<string>& tokens,
                      const string& context,
                      const string& link_creator_tag,
                      BaseProxy::ORCHESTRATION_SCHEMA_TYPE orchestration,
                      const shared_ptr<LinkCreator> link_creator = shared_ptr<LinkCreator>(nullptr));

    ~LinkCreationProxy();

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
     * Create or update one or more links using the passed QueryAnswer.
     *
     * @param answer QueryAnswer to be used in link creation
     * @return a LinkCreationStats object with statistics about actual link creation
     */
    LinkCreationStats link_creation(shared_ptr<QueryAnswer> answer);

    /**
     * Return true iff the one or more of the stop criteria have been met.
     *
     * @return true iff the one or more of the stop criteria have been met.
     */
    bool stop_criteria_met();

    /**
     * Increments the count of rounds by 1.
     */
    void inc_round_count();

    /**
     * Returns a string representation with all command parameter values.
     *
     * @return a string representation with all command parameter values.
     */
    virtual string to_string();

    /**
     * Returns true iff the link creation function is supposed to be evaluated remotely.
     *
     * @return true iff the link creation function is supposed to be evaluated remotely.
     */
    bool is_link_creation_function_remote();

    /**
     * Call Attention Broker to set determiners according to a state buffer with accumulated
     * requests from one entire link creation round. This buffer is emptied as a side effect.
     */
    void flush_determiners();

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
     * Packs mandatory LCA parameters in token array.
     */
    void pack_command_line_args() override;

    /**
     * Add LCA proxy tokens to output.
     */
    void tokenize(vector<string>& output) override;

   private:
    void set_link_creator_function_tag(const string& tag);
    void init();

    shared_ptr<LinkCreator> link_creation_function_object;
    mutex api_mutex;
    string link_creator_function_tag;
    bool ongoing_remote_link_creation;
    unsigned int round_count;
};

}  // namespace link_creation_agent
