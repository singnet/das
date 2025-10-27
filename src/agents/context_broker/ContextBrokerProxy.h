#pragma once

#include <fstream>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include "BaseQueryProxy.h"
#include "QueryAnswer.h"

using namespace std;
using namespace service_bus;
using namespace query_engine;
using namespace agents;

namespace context_broker {

/**
 * Proxy which allows communication between the caller of the CONTEXT_BROKER command and
 * the bus element actually executing it.
 *
 * The caller can use this object in order to iterate through results of the command and to
 * abort the command execution before it finished.
 *
 * On the command processor side, this object is used to retrieve command parameters (e.g.
 * the actual query tokens, flags etc).
 */
class ContextBrokerProxy : public BaseQueryProxy {
   public:
    // ---------------------------------------------------------------------------------------------
    // Constructors, destructors and static state

    // Proxy Commands
    static string ATTENTION_BROKER_SET_PARAMETERS;
    static string ATTENTION_BROKER_SET_PARAMETERS_FINISHED;
    static string CONTEXT_CREATED;

    // Query command's optional parameters
    static string USE_CACHE;                          // Whether to use cache
    static string ENFORCE_CACHE_RECREATION;           // Whether to enforce cache recreation
    static string INITIAL_RENT_RATE;                  // AttentionBroker rent rate
    static string INITIAL_SPREADING_RATE_LOWERBOUND;  // AttentionBroker lowerbound spreading rate
    static string INITIAL_SPREADING_RATE_UPPERBOUND;  // AttentionBroker upperbound spreading rate

    // Default values for AttentionBrokerClient::set_parameters()
    static double DEFAULT_RENT_RATE;
    static double DEFAULT_SPREADING_RATE_LOWERBOUND;
    static double DEFAULT_SPREADING_RATE_UPPERBOUND;

    /**
     * Empty constructor typically used on server side.
     */
    ContextBrokerProxy();

    /**
     * Constructor for query-based context.
     * @param name Context name
     * @param query Query tokens
     * @param determiner_schema Determiner schema mapping
     * @param stimulus_schema Stimulus schema
     */
    ContextBrokerProxy(const string& name,
                       const vector<string>& query,
                       const vector<pair<QueryAnswerElement, QueryAnswerElement>>& determiner_schema,
                       const vector<QueryAnswerElement>& stimulus_schema);

    /**
     * Destructor.
     */
    virtual ~ContextBrokerProxy();

    // ---------------------------------------------------------------------------------------------
    // Context-specific API

    /**
     * Get the context created flag.
     * @return Whether the context has been created
     */
    bool is_context_created();

    virtual bool from_remote_peer(const string& command, const vector<string>& args) override;

    /**
     * Set the attention broker parameters.
     * @param rent_rate The rent rate.
     * @param spreading_rate_lowerbound The spreading rate lowerbound.
     * @param spreading_rate_upperbound The spreading rate upperbound.
     */
    void attention_broker_set_parameters(double rent_rate,
                                         double spreading_rate_lowerbound,
                                         double spreading_rate_upperbound);
    /**
     * Get the attention broker parameters finished flag.
     * @return Whether the attention broker parameters have been set
     */
    bool attention_broker_set_parameters_finished();

    /**
     * Get the context name.
     * @return Context name
     */
    const string& get_name();

    /**
     * Get the context key.
     * @return Context key
     */
    const string& get_key();

    /**
     * Get the determiner schema.
     * @return Vector of determiner schema pairs
     */
    const vector<pair<QueryAnswerElement, QueryAnswerElement>>& get_determiner_schema();

    /**
     * Get the stimulus schema.
     * @return Vector of stimulus schema elements
     */
    const vector<QueryAnswerElement>& get_stimulus_schema();

    /**
     * Get the cache file name.
     * @return Cache file name
     */
    const string& get_cache_file_name();

    /**
     * Get the use_cache flag.
     * @return Whether to use cache
     */
    bool get_use_cache();

    /**
     * Get the enforce_cache_recreation flag.
     * @return Whether to enforce cache recreation
     */
    bool get_enforce_cache_recreation();

    /**
     * Get the initial rent rate.
     * @return Rent rate
     */
    double get_initial_rent_rate();

    /**
     * Get the initial spreading rate lowerbound.
     * @return Spreading rate lowerbound
     */
    double get_initial_spreading_rate_lowerbound();

    /**
     * Get the initial spreading rate upperbound.
     * @return Spreading rate upperbound
     */
    double get_initial_spreading_rate_upperbound();

    // ---------------------------------------------------------------------------------------------
    // BaseQueryProxy overrides

    virtual void tokenize(vector<string>& output) override;
    virtual void untokenize(vector<string>& tokens) override;
    virtual string to_string() override;
    virtual void pack_command_line_args() override;

    map<string, unsigned int> to_stimulate;
    vector<vector<string>> determiner_request;

    // Attention broker parameters
    bool update_attention_broker_parameters;
    double rent_rate;
    double spreading_rate_lowerbound;
    double spreading_rate_upperbound;

   private:
    void init(const string& name);
    void set_default_query_parameters();

    // Context-specific members
    mutex api_mutex;
    string name;
    string key;
    string cache_file_name;

    vector<pair<QueryAnswerElement, QueryAnswerElement>> determiner_schema;
    vector<QueryAnswerElement> stimulus_schema;

    // Control flags
    bool context_created;
    bool ongoing_attention_broker_set_parameters;
};

}  // namespace context_broker
