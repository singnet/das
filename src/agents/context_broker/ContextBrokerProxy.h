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

    /**
     * Empty constructor typically used on server side.
     * @param use_cache Whether to use cache
     */
    ContextBrokerProxy(bool use_cache = true);

    /**
     * Constructor for query-based context.
     * @param name Context name
     * @param query Query tokens
     * @param determiner_schema Determiner schema mapping
     * @param stimulus_schema Stimulus schema
     * @param use_cache Whether to use cache
     * @param rent_rate AttentionBroker rent rate
     * @param spreading_rate_lowerbound AttentionBroker lowerbound spreading rate
     * @param spreading_rate_upperbound AttentionBroker upperbound spreading rate
     */
    ContextBrokerProxy(const string& name,
                       const vector<string>& query,
                       const vector<pair<QueryAnswerElement, QueryAnswerElement>>& determiner_schema,
                       const vector<QueryAnswerElement>& stimulus_schema,
                       bool use_cache,
                       float rent_rate,
                       float spreading_rate_lowerbound,
                       float spreading_rate_upperbound);

    /**
     * Destructor.
     */
    virtual ~ContextBrokerProxy();

    void from_atomdb(const string& atom_handle);

    // ---------------------------------------------------------------------------------------------
    // Context-specific API

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
     * Get the atom handle.
     * @return Atom handle
     */
    const string& get_atom_handle();

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
     * Get the determiner request.
     * @return Vector of determiner request vectors
     */
    vector<vector<string>>& get_determiner_request();

    /**
     * Get the to_stimulate map.
     * @return Map of strings to unsigned ints
     */
    map<string, unsigned int>& get_to_stimulate();

    /**
     * Clear the to_stimulate map.
     */
    void clear_to_stimulate();

    /**
     * Clear the determiner request.
     */
    void clear_determiner_request();

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
     * Get the rent rate.
     * @return Rent rate
     */
    float get_rent_rate();

    /**
     * Get the spreading rate lowerbound.
     * @return Spreading rate lowerbound
     */
    float get_spreading_rate_lowerbound();

    /**
     * Get the spreading rate upperbound.
     * @return Spreading rate upperbound
     */
    float get_spreading_rate_upperbound();

    // ---------------------------------------------------------------------------------------------
    // BaseQueryProxy overrides

    virtual void tokenize(vector<string>& output) override;
    virtual void untokenize(vector<string>& tokens) override;
    virtual string to_string() override;
    virtual void pack_command_line_args() override;

   private:
    void init(const string& name, bool use_cache);

    // Context-specific members
    mutex api_mutex;
    string name;
    string key;
    string cache_file_name;
    string atom_handle;
    bool use_cache;
    map<string, unsigned int> to_stimulate;
    vector<vector<string>> determiner_request;
    vector<pair<QueryAnswerElement, QueryAnswerElement>> determiner_schema;
    vector<QueryAnswerElement> stimulus_schema;
    float rent_rate;
    float spreading_rate_lowerbound;
    float spreading_rate_upperbound;
};

}  // namespace context_broker
