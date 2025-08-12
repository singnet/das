#pragma once

#include <map>
#include <memory>
#include <thread>

#include "AtomSpace.h"
#include "BusCommandProcessor.h"
#include "PatternMatchingQueryProxy.h"
#include "QueryEvolutionProxy.h"
#include "StoppableThread.h"

using namespace std;
using namespace service_bus;
using namespace atomspace;

namespace evolution {

/**
 * Bus element responsible for processing QUERY_EVOLUTION commands.
 */
class QueryEvolutionProcessor : public BusCommandProcessor {
   public:
    QueryEvolutionProcessor();
    ~QueryEvolutionProcessor();

    // ---------------------------------------------------------------------------------------------
    // Virtual BusCommandProcessor API

    /**
     * Returns an empty instance of the QueryEvolutionProxy.
     *
     * @return An empty instance of the QueryEvolutionProxy.
     */
    virtual shared_ptr<BusCommandProxy> factory_empty_proxy();

    /**
     * Method which is called when a command owned by this processor is issued in the bus.
     */
    virtual void run_command(shared_ptr<BusCommandProxy> proxy);

   protected:
    // Protected to ease writing tests
    void select_best_individuals(shared_ptr<QueryEvolutionProxy> proxy,
                                 vector<std::pair<shared_ptr<QueryAnswer>, float>>& population,
                                 vector<std::pair<shared_ptr<QueryAnswer>, float>>& selected);

    void select_one_by_tournament(shared_ptr<QueryEvolutionProxy> proxy,
                                  vector<std::pair<shared_ptr<QueryAnswer>, float>>& population,
                                  vector<std::pair<shared_ptr<QueryAnswer>, float>>& selected);

    void apply_elitism(shared_ptr<QueryEvolutionProxy> proxy,
                       vector<std::pair<shared_ptr<QueryAnswer>, float>>& population,
                       vector<std::pair<shared_ptr<QueryAnswer>, float>>& selected);

    void evolve_query(shared_ptr<StoppableThread> monitor, shared_ptr<QueryEvolutionProxy> proxy);

    void sample_population(shared_ptr<StoppableThread> monitor,
                           shared_ptr<QueryEvolutionProxy> proxy,
                           vector<std::pair<shared_ptr<QueryAnswer>, float>>& population);

    void update_attention_allocation(shared_ptr<QueryEvolutionProxy> proxy,
                                     vector<std::pair<shared_ptr<QueryAnswer>, float>>& selected);

   private:
    shared_ptr<PatternMatchingQueryProxy> issue_sampling_query(shared_ptr<QueryEvolutionProxy> proxy, bool attention_flag);
    shared_ptr<PatternMatchingQueryProxy> issue_correlation_query(shared_ptr<QueryEvolutionProxy> proxy, vector<string> query_tokens);
    void correlate_similar(shared_ptr<QueryEvolutionProxy> proxy,
                           shared_ptr<QueryAnswer> correlation_query_answer);
    void stimulate(shared_ptr<QueryEvolutionProxy> proxy,
                   vector<std::pair<shared_ptr<QueryAnswer>, float>>& selected);
    void thread_process_one_query(shared_ptr<StoppableThread>, shared_ptr<QueryEvolutionProxy> proxy);
    void remove_query_thread(const string& stoppable_thread_id);

    map<string, shared_ptr<StoppableThread>> query_threads;
    mutex query_threads_mutex;
    shared_ptr<QueryEvolutionProxy> proxy;
    AtomSpace atom_space;
};

}  // namespace evolution
