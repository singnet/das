#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "Assignment.h"
#include "AtomDB.h"
#include "LinkSchema.h"
#include "QueryElement.h"
#include "Source.h"
#include "StoppableThread.h"
#include "ThreadSafeHashmap.h"

using namespace std;
using namespace query_engine;
using namespace query_element;
using namespace atoms;
using namespace atomdb;

#define MAX_GET_IMPORTANCE_BUNDLE_SIZE ((unsigned int) 100000)

namespace query_element {

/**
 * A QueryElement which represents terminals (i.e. Nodes, Links and Variables) in the query tree.
 */
class LinkTemplate : public QueryElement {
   private:
    class SourceElement : public Source {
       public:
        bool buffers_set_up_flag;
        mutex api_mutex;
        SourceElement() { buffers_set_up_flag = false; }
        string get_attention_broker_address() { return this->attention_broker_address; }
        void add_handle(char* handle, float importance, Assignment& assignment) {
            QueryAnswer* answer = new QueryAnswer(strdup(handle), importance);
            answer->assignment.copy_from(assignment);
            this->output_buffer->add_query_answer(answer);
        }
        void query_answers_finished() { this->output_buffer->query_answers_finished(); }
        void setup_buffers() override {
            lock_guard<mutex> semaphore(this->api_mutex);
            Source::setup_buffers();
            this->buffers_set_up_flag = true;
        }
        bool buffers_set_up() {
            lock_guard<mutex> semaphore(this->api_mutex);
            return this->buffers_set_up_flag;
        }
    };

    vector<shared_ptr<QueryElement>> targets;
    string context;
    bool positive_importance_flag;
    bool use_cache;
    bool inner_flag;
    LinkSchema link_schema;
    shared_ptr<SourceElement> source_element;
    shared_ptr<StoppableThread> processor;
    static ThreadSafeHashmap<string, shared_ptr<atomdb_api_types::HandleSet>> cache;

    void recursive_build(shared_ptr<QueryElement> element, LinkSchema& link_schema);
    void compute_importance(vector<pair<char*, float>>& handles);
    void processor_method(shared_ptr<StoppableThread> monitor);
    void start_thread();

   public:
    ~LinkTemplate();
    LinkTemplate(const string& type,
                 const vector<shared_ptr<QueryElement>>& targets,
                 const string& context,
                 bool positive_importance_flag,
                 bool use_cache);

    void build();
    string to_string();
    string get_handle();
    shared_ptr<Source> get_source_element();

    static unsigned int next_instance_count() {
        static unsigned int instance_count = 0;
        return instance_count++;
    }

    static void clear_cache() { cache.clear(); }

    static ThreadSafeHashmap<string, shared_ptr<atomdb_api_types::HandleSet>>& fetched_links_cache() {
        return cache;
    }

    const string& get_type();

    // QueryElement virtual API

    virtual void setup_buffers() {}

    /**
     * Empty implementation. There are no QueryNode element or local thread to shut down.
     */
    virtual void graceful_shutdown() {}
};
}  // namespace query_element
