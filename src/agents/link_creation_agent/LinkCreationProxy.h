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

    // Commands allowed at the proxy level (caller <--> processor)
    static string PROCESS_QUERY_ANSWER;  // Delivers a bundle with QueryAnswer objects to process
    static string PROCESS_QUERY_ANSWER_RESPONSE;  // Delivers the answer for a previous
                                                  // PROCESS_QUERY_ANSWER command

    // LCA command's optional parameters
    static string MAX_SUCCESSFUL_CREATES_PER_ROUND;
    static string MAX_ROUNDS;
    static string MAX_VISITS_PER_ROUND;
    static string MAX_UNPRODUCTIVE_ANSWERS_PER_ROUND;

    LinkCreationProxy();

    LinkCreationProxy(const vector<string>& tokens,
                      const string& context,
                      const string& link_creator_tag,
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
     * Create or update one or more links using the passed QueryAnswer. The actual number of links
     * <created, updated> is returned in a pair.
     *
     * @param answer QueryAnswer to be used in link creation
     */
    pair<unsigned int, unsigned int> link_creation(shared_ptr<QueryAnswer> answer);

    /**
     * Return true iff the one or more of the stop criteria have been met.
     *
     * @return true iff the one or more of the stop criteria have been met.
     */
    bool stop_criteria_met();

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
     * Send a request to peer in order to execute the link creator function
     * aswer bundle
     */
    void remote_link_creation(const vector<string>& answer_bundle);

    /**
     * Returns a vector of statistics about remotely created links.
     *
     * @return a vector of statistics about remotely created links.
     */
    vector<pair<unsigned int, unsigned int>> get_remotely_created_links();

    /**
     * Returns true iff there's no remote link creator function is going on
     *
     * @return true iff there's no remote link creator function is going on
     */
    bool remote_link_creation_finished();

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
     * Remotelly create links using QueryAnswers
     *
     * @param args a bundle of tokenized QueryAnswers.
     */
    void process_query_answer(const vector<string>& args);

    /**
     * Response of a create_links() command
     *
     * @param args a bundle of <unsigned int, unsigned int> pairs
     */
    void process_query_answer_response(const vector<string>& args);

    /**
     * Packs mandatory LCA parameters in token array.
     */
    void pack_command_line_args() override;

    /**
     * Add LCA proxy tokens to output.
     */
    void tokenize(vector<string>& output) override;

   private:
    void set_default_query_parameters();
    void set_link_creator_function_tag(const string& tag);
    void init();

    shared_ptr<LinkCreator> link_creation_function_object;
    mutex api_mutex;
    string link_creator_function_tag;
    bool ongoing_remote_link_creation;
    vector<pair<unsigned int, unsigned int>> remote_link_creation_result;
    unsigned int round_count;
};

}  // namespace link_creation_agent
