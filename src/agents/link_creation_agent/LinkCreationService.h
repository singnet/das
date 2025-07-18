/**
 * @file LinkCreationService.h
 * @brief LCALink Creation Service class
 */
#pragma once

#include <mutex>
#include <set>
#include <shared_mutex>

#include "EquivalenceProcessor.h"
#include "ImplicationProcessor.h"
#include "LCALink.h"
#include "LCAQueue.h"
#include "LinkProcessor.h"
#include "PatternMatchingQueryProxy.h"
#include "QueryAnswer.h"
#include "ServiceBusSingleton.h"
#include "TemplateProcessor.h"
#include "ThreadPool.h"

using namespace query_engine;
using namespace std;
namespace link_creation_agent {
/**
 * @class LinkCreationService
 * @brief Service class for creating links using a thread pool.
 *
 * This class provides functionality to process requests for link creation
 * using a thread pool. It manages the processing of remote iterators and
 * ensures that links are created by blocking the client until the link
 * creation is complete.
 *
 * @note The processed_link_handles data structure can be changed to a better
 * data structure in the future.
 */
class LinkCreationService

{
   public:
    LinkCreationService(int thread_count);
    /**
     * @brief Add an iterator to process in thread pool
     * @param proxy PatternMatchingQueryProxy object
     * @param das_client DAS LCANode client
     */
    void process_request(shared_ptr<PatternMatchingQueryProxy> proxy,
                         vector<string>& link_template,
                         const string& context,
                         const string& request_id,
                         int max_query_answers);

    void set_timeout(int timeout);
    void set_metta_file_path(string metta_file_path);
    void set_save_links_to_metta_file(bool save_links_to_metta_file) {
        this->save_links_to_metta_file = save_links_to_metta_file;
    }
    void set_save_links_to_db(bool save_links_to_db) { this->save_links_to_db = save_links_to_db; }
    /**
     * @brief Destructor
     */
    ~LinkCreationService();

   private:
    ThreadPool thread_pool;
    // this can be changed to a better data structure
    set<string> processed_link_handles;
    string metta_file_path;
    shared_mutex m_mutex;
    shared_ptr<LinkTemplateProcessor> link_template_processor;
    shared_ptr<ImplicationProcessor> implication_processor;
    shared_ptr<EquivalenceProcessor> equivalence_processor;
    shared_ptr<mutex> query_agent_mutex;
    Queue<tuple<string, vector<string>>> link_creation_queue;
    bool is_stoping = false;
    thread create_link_thread;
    set<string> metta_expression_set;
    bool save_links_to_metta_file = false;
    bool save_links_to_db = false;

    int timeout = 300 * 1000;

    /**
     * @brief Create a link, blocking the client until the link is created
     * @param link LCALink object
     * @param das_client DAS LCANode client
     */
    void create_links();
    vector<vector<string>> process_query_answer(shared_ptr<QueryAnswer> query_answer,
                                                vector<string> params,
                                                vector<string> link_template);
    void enqueue_link_creation_request(const string& request_id,
                                       const vector<vector<string>>& link_tokens);
};

}  // namespace link_creation_agent
