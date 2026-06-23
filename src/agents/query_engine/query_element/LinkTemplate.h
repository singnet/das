#pragma once

#include <mutex>
#include <random>
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

namespace query_element {

/**
 * A QueryElement which represents terminals (i.e. Nodes, Links and Variables) in the query tree.
 */
class LinkTemplate : public QueryElement {
   private:
    enum AttentionFocusStrategy { UNDEFINED = 0, PERCENTAGE };
    class AttentionFocusRecord {
       public:
        AttentionFocusRecord(char* handle,
                             float importance,
                             const Assignment& assignment,
                             const map<string, string>& metta_expression) {
            this->handle = handle;
            this->importance = importance;
            this->assignment = assignment;
            this->metta_expression = metta_expression;
        }
        AttentionFocusRecord(const AttentionFocusRecord& other) = default;
        AttentionFocusRecord& operator=(const AttentionFocusRecord& other) = default;
        char* handle;
        float importance;
        Assignment assignment;
        map<string, string> metta_expression;
    };
    class SourceElement : public Source {
       public:
        bool buffers_set_up_flag;
        mutex api_mutex;
        SourceElement() { buffers_set_up_flag = false; }
        void add_handle(char* handle,
                        float importance,
                        const Assignment& assignment,
                        map<string, string> metta_expression = {}) {
            QueryAnswer* answer = new QueryAnswer(string(handle), importance);
            answer->assignment = assignment;
            answer->metta_expression = metta_expression;
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
    double attention_focus_strictness;
    bool positive_importance_flag;
    bool disregard_importance_flag;
    bool unique_value_flag;
    bool use_cache;
    bool inner_flag;
    LinkSchema link_schema;
    shared_ptr<SourceElement> source_element;
    shared_ptr<StoppableThread> processor;
    std::mt19937* random_generator;
    AttentionFocusStrategy attention_focus_strategy;
    static ThreadSafeHashmap<string, shared_ptr<atomdb_api_types::HandleSet>> cache;

    unsigned int report_attention_focus_by_percentage(
        vector<AttentionFocusRecord>& attention_focus_candidates);
    unsigned int report_attention_focus(vector<AttentionFocusRecord>& attention_focus_candidates);

    void recursive_build(shared_ptr<QueryElement> element, LinkSchema& link_schema);
    void compute_importance(vector<pair<char*, float>>& handles);
    void processor_method(shared_ptr<StoppableThread> monitor);
    void start_thread();

   public:
    ~LinkTemplate();
    /**
     * Constructor.
     *
     * @param type Type of the underlying LinkSchema.
     * @param targets Target elements attached to this LinkTemplate.
     * @param context Attention context used to query the AttentionBroker
     * @param attention_focus_strictness Measure of how strict AttentionFocus will be considered, prunning fetched/matched handles before reporting them.
     * @param positive_importance_flag If true, only handles eith STI > 0 will be reported.
     * @param disregard_importance_flag If true, STI os the handles is disregarded (STI values are not even obtained).
     * @param unique_value_flag If true, prevent the same value from being assigned to 2 different variables in the LinkTemplate.
     * @param use_cache If true, a cache for fetched elements will be used and maintained.
     */
    LinkTemplate(const string& type,
                 const vector<shared_ptr<QueryElement>>& targets,
                 const string& context,
                 double attention_focus_strictness,
                 bool positive_importance_flag,
                 bool disregard_importance_flag,
                 bool unique_value_flag,
                 bool use_cache);

    /**
     * This method is used when a LinkTemplate is build incrementally e.g. by parsing a
     * MeTTa expression. It indicates that the building process is finished and that
     * the internal fields can be calculated.
     *
     * @return a string representation of this LinkTemplate
     */
    void build();

    /**
     * Compute and return a string representation of this LinkTemplate
     *
     * @return a string representation of this LinkTemplate
     */
    string to_string();

    /**
     * Compute and return the handle of this LinkTemplate.
     *
     * @return the handle of this LinkTemplate.
     */
    string get_handle();

    /**
     * Return the DistributedAlgorithmNode which is the actual "source element" of this LinkTemplate.
     * This element is ther actual one used in the query tree.
     *
     * @return the DistributedALgorithmNode which is the actual "source element" of this LinkTemplate.
     */
    shared_ptr<Source> get_source_element();

    /**
     * Return a global incremental index to assemble the name of the LinkTemplate object used as id in the DistributedAlgorithmNode.
     *
     * @return a global incremental index to assemble the name of the LinkTemplate object used as id in the DistributedAlgorithmNode.
     */
    static unsigned int next_instance_count() {
        static unsigned int instance_count = 0;
        return instance_count++;
    }

    /**
     * Clear the global LinkTemplate cache.
     */
    static void clear_cache() { cache.clear(); }

    /**
     * Return the global LinkTemplate cache.
     *
     * @return the global LinkTemplate cache.
     */
    static ThreadSafeHashmap<string, shared_ptr<atomdb_api_types::HandleSet>>& fetched_links_cache() {
        return cache;
    }

    /**
     * Return the underlying LinkSchema type
     *
     * @return the underlying LinkSchema type
     */
    const string& get_type();

    // QueryElement virtual API

    /**
     * Empty implementation. There are no QueryNode element or local thread to shut down.
     */
    virtual void setup_buffers() {}

    /**
     * Empty implementation. There are no QueryNode element or local thread to shut down.
     */
    virtual void graceful_shutdown() {}
};
}  // namespace query_element
