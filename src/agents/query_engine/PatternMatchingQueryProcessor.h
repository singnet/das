#pragma once

#include <map>
#include <memory>
#include <stack>
#include <thread>

#include "BusCommandProcessor.h"
#include "PatternMatchingQueryProxy.h"
#include "QueryElement.h"
#include "QueryElementRegistry.h"
#include "Sink.h"
#include "StoppableThread.h"

#define ATTENTION_BROKER_ADDRESS "localhost:37007"
#define MAX_BUNDLE_SIZE ((unsigned int) 10000)

using namespace std;
using namespace service_bus;
using namespace query_element;

namespace atomdb {

/**
 * Bus element responsible for processing PATTERN_MATCHING_QUERY commands.
 */
class PatternMatchingQueryProcessor : public BusCommandProcessor {
   public:
    PatternMatchingQueryProcessor();
    ~PatternMatchingQueryProcessor();

    // ---------------------------------------------------------------------------------------------
    // Virtual BusCommandProcessor API

    /**
     * Returns an empty instance of the PatternMatchingQueryProxy.
     *
     * @return An empty instance of the PatternMatchingQueryProxy.
     */
    virtual shared_ptr<BusCommandProxy> factory_empty_proxy();

    /**
     * Method which is called when a command owned by this processor is issued in the bus.
     */
    virtual void run_command(shared_ptr<BusCommandProxy> proxy);

   private:
    void update_attention_broker_single_answer(shared_ptr<PatternMatchingQueryProxy> proxy,
                                               QueryAnswer* answer,
                                               set<string>& joint_answer);
    void update_attention_broker_joint_answer(shared_ptr<PatternMatchingQueryProxy> proxy,
                                              set<string>& joint_answer);
    void process_query_answers(shared_ptr<PatternMatchingQueryProxy> proxy,
                               shared_ptr<Sink> query_sink,
                               set<string>& joint_answer,
                               unsigned int& answer_count);
    shared_ptr<QueryElement> setup_query_tree(shared_ptr<PatternMatchingQueryProxy> proxy,
                                              QueryElementRegistry* query_element_registry);
    void thread_process_one_query(shared_ptr<StoppableThread>,
                                  shared_ptr<PatternMatchingQueryProxy> proxy);
    shared_ptr<QueryElement> build_link_template(shared_ptr<PatternMatchingQueryProxy> proxy,
                                                 unsigned int cursor,
                                                 stack<shared_ptr<QueryElement>>& element_stack,
                                                 QueryElementRegistry* query_element_registry);

    shared_ptr<QueryElement> build_link_template2(shared_ptr<PatternMatchingQueryProxy> proxy,
                                                  unsigned int cursor,
                                                  stack<shared_ptr<QueryElement>>& element_stack);

    shared_ptr<QueryElement> build_and(shared_ptr<PatternMatchingQueryProxy> proxy,
                                       unsigned int cursor,
                                       stack<shared_ptr<QueryElement>>& element_stack);

    shared_ptr<QueryElement> build_or(shared_ptr<PatternMatchingQueryProxy> proxy,
                                      unsigned int cursor,
                                      stack<shared_ptr<QueryElement>>& element_stack);

    shared_ptr<QueryElement> build_link(shared_ptr<PatternMatchingQueryProxy> proxy,
                                        unsigned int cursor,
                                        stack<shared_ptr<QueryElement>>& element_stack);

    shared_ptr<QueryElement> build_unique_assignment_filter(
        shared_ptr<PatternMatchingQueryProxy> proxy,
        unsigned int cursor,
        stack<shared_ptr<QueryElement>>& element_stack);

    void remove_query_thread(const string& stoppable_thread_id);

    map<string, shared_ptr<StoppableThread>> query_threads;
    mutex query_threads_mutex;
    shared_ptr<PatternMatchingQueryProxy> proxy;
};

}  // namespace atomdb
