#pragma once

#include <mutex>

#include "BaseProxy.h"
#include "FitnessFunction.h"
#include "Message.h"
#include "QueryAnswer.h"
#include "SharedQueue.h"

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
class QueryEvolutionProxy : public BaseProxy {
   public:
    // ---------------------------------------------------------------------------------------------
    // Constructors, destructors and static state

    // Commands allowed at the proxy level (caller <--> processor)
    static string ANSWER_BUNDLE;  // Delivery of a bundle with QueryAnswer objects

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
                        const string& fitness_function,
                        const string& context);

    /**
     * Destructor.
     */
    virtual ~QueryEvolutionProxy();

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
    std::pair<shared_ptr<QueryAnswer>, float> pop();

    /**
     * Returns the number of QueryAnswers delivered so far.
     *
     * NOTE: the count is for delivered answers not iterated ones.
     *
     * @return The number of QueryAnswers delivered so far.
     */
    unsigned int get_count();

    // ---------------------------------------------------------------------------------------------
    // Server-side API

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
     * Pack query arguments into args vector
     */
    void pack_custom_args();

    /**
     * Compute the fitness value of the passed QueryAnswer.
     *
     * @param answer QueryAnswer to whose fitness value is to be computed.
     * @return The fitness value of the passed QueryAnswer.
     */
    float compute_fitness(shared_ptr<QueryAnswer> answer);

    /**
     * Returns a string representation with all command parameter values.
     *
     * @return a string representation with all command parameter values.
     */
    string to_string();

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
     * Getter for fitness_function_tag
     *
     * fitness_function_tag is the id for the fitness function to be used to evaluate query answers
     *
     * @return fitness_function_tag
     */
    const string& get_fitness_function_tag();

    /**
     * Setter for fitness_function_tag
     *
     * fitness_function_tag is the id for the fitness function to be used to evaluate query answers
     *
     * @param flag Flag
     */
    void set_fitness_function_tag(const string& tag);

    /**
     * Getter for population_size
     *
     * population_size Number of answers sampled every evolution cycle.
     *
     * @return population_size
     */
    unsigned int get_population_size();

    /**
     * Setter for population_size
     *
     * population_size Number of answers sampled every evolution cycle.
     *
     * @param population_size Population size
     */
    void set_population_size(unsigned int population_size);

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

    vector<string> query_tokens;

   private:
    void init();
    void set_default_query_parameters();

    mutex api_mutex;
    SharedQueue answer_queue;
    unsigned int answer_count;
    string context;
    shared_ptr<FitnessFunction> fitness_function_object;

    // ---------------------------------------------------------------------------------------------
    // Query parameters

    /**
     * When true, query operators (e.g. And, Or) don't output more than one
     * QueryAnswer with the same variable assignment.
     * */
    bool unique_assignment_flag;

    /**
     * Fitness function selector.
     */
    string fitness_function_tag;

    /**
     * Number of answers sampled every evolution cycle.
     */
    unsigned int population_size;
};

}  // namespace evolution
