#pragma once

#include <mutex>

#include "BaseQueryProxy.h"
#include "FitnessFunction.h"

using namespace std;
using namespace service_bus;
using namespace query_engine;
using namespace agents;
using namespace fitness_functions;

namespace evolution {

/**
 * Proxy which allows communication between the caller of the QUERY_EVOLUTION command and
 * the bus element actually executing it.
 *
 * The caller can use this object in order to iterate through results of the command and to
 * abort the command execution before it finished.
 *
 * On the command processor side, this object is used to retrieve command parameters (e.g.
 * the actual query tokens, flags etc).
 */
class QueryEvolutionProxy : public BaseQueryProxy {
   public:
    // ---------------------------------------------------------------------------------------------
    // Constructors, destructors and static state

    // Commands allowed at the proxy level (caller <--> processor)
    static string EVAL_FITNESS;  // Delivery of a bundle with QueryAnswer objects to evaluate fitness
    static string EVAL_FITNESS_RESPONSE;  // Delivery of the answer of a EVAL_FITNESS command

    // Query command's optional parameters
    static string POPULATION_SIZE;
    static string MAX_GENERATIONS;
    static string ELITISM_RATE;            // Rate on POPULATION_SIZE
    static string SELECTION_RATE;          // Rate on POPULATION_SIZE
    static string TOTAL_ATTENTION_TOKENS;  // Used in attention allocation update

    /**
     * Empty constructor typically used on server side.
     */
    QueryEvolutionProxy();

    /**
     * Basic constructor typically used on client side.
     *
     * @param tokens Query tokens.
     * @param context AttentionBroker context
     */
    QueryEvolutionProxy(const vector<string>& tokens,
                        const vector<string>& correlation_tokens,
                        const vector<string>& correlation_variables,
                        const string& context,
                        const string& fitness_function_tag,
                        const shared_ptr<FitnessFunction> fitness_function = shared_ptr<FitnessFunction>(nullptr));

    /**
     * Destructor.
     */
    virtual ~QueryEvolutionProxy();

    // ---------------------------------------------------------------------------------------------
    // Client-side API

    /**
     * Builds the args vector to be passed in the RPC
     */
    virtual void pack_command_line_args();

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
     * Compute the fitness value of the passed QueryAnswer.
     *
     * @param answer QueryAnswer to whose fitness value is to be computed.
     * @return The fitness value of the passed QueryAnswer.
     */
    float compute_fitness(shared_ptr<QueryAnswer> answer);

    /**
     * Return true iff the one or more of the stop criteria have been met.
     *
     * @return true iff the one or more of the stop criteria have been met.
     */
    bool stop_criteria_met();

    /**
     * Perform actions after a new population is sampled such as reporting any good
     * new asnwers to the client caller etc.
     */
    void new_population_sampled(vector<std::pair<shared_ptr<QueryAnswer>, float>>& population);

    /**
     * Returns a string representation with all command parameter values.
     *
     * @return a string representation with all command parameter values.
     */
    virtual string to_string();

    /**
     * Returns true iff the fitness function is supposed to be evaluated remotely.
     *
     * @return true iff the fitness function is supposed to be evaluated remotely.
     */
    bool is_fitness_function_remote();

    /**
     * Send a request to peer in order to evaluate fitness function of the passed
     * aswer bundle
     */
    void remote_fitness_evaluation(const vector<string>& answer_bundle);

    /**
     * Returns a vector of fitness values which have been evaliuated on the remote proxy.
     *
     * @return a vector of fitness values which have been evaliuated on the remote proxy.
     */
    vector<float> get_remotely_evaluated_fitness();

    /**
     * Returns true iff there's no remote fitness evaluation going on.
     *
     * @return true iff there's no remote fitness evaluation going on.
     */
    bool remote_fitness_evaluation_finished();

    const vector<string>& get_correlation_tokens();
    const vector<string>& get_correlation_variables();

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
     * Remotelly evaluate the fitness function of QueryAnswers
     *
     * @param args a bundle of tokenized QueryAnswers.
     */
    void eval_fitness(const vector<string>& args);

    /**
     * Response of a eval_fitness() command
     *
     * @param args a bundle of fitness values.
     */
    void eval_fitness_response(const vector<string>& args);


   private:
    void set_default_query_parameters();
    void set_fitness_function_tag(const string& tag);
    void init();

    mutex api_mutex;
    shared_ptr<FitnessFunction> fitness_function_object;
    string fitness_function_tag;
    float best_reported_fitness;
    unsigned int num_generations;
    vector<string> correlation_tokens;
    vector<string> correlation_variables;
    bool ongoing_remote_fitness_evaluation;
    vector<float> remote_fitness_evaluation_result;
};

}  // namespace evolution
