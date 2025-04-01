/**
 * @file link_creation_service.h
 * @brief Link Creation Service class
 */
#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <set>

#include "HandlesAnswer.h"
#include "RemoteIterator.h"
#include "das_agent_node.h"
#include "link.h"
#include "thread_pool.h"
#include "link_processor.h"
#include "template_processor.h"
#include "equivalence_processor.h"
#include "implication_processor.h"
#include "DASNode.h"


#define DEBUG

using namespace das_agent;
using namespace query_node;
using namespace query_element;
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
    LinkCreationService(int thread_count, shared_ptr<DASNode> das_node);
    /**
     * @brief Add an iterator to process in thread pool
     * @param iterator RemoteIterator object
     * @param das_client DAS Node client
     */
    void process_request(shared_ptr<RemoteIterator<HandlesAnswer>> iterator,
                         DasAgentNode* das_client,
                         vector<string>& link_template,
                         int max_query_answers);

    void set_timeout(int timeout);
    /**
     * @brief Destructor
     */
    ~LinkCreationService();

   private:
    ThreadPool thread_pool;
    // this can be changed to a better data structure
    set<string> processed_link_handles;
    std::mutex m_mutex;
    std::condition_variable m_cond;
    shared_ptr<LinkTemplateProcessor> link_template_processor;
    shared_ptr<ImplicationProcessor> implication_processor;
    shared_ptr<EquivalenceProcessor> equivalence_processor;

    int timeout = 60;

    /**
     * @brief Create a link, blocking the client until the link is created
     * @param link Link object
     * @param das_client DAS Node client
     */
    void create_link(std::vector<std::vector<std::string>>& links, DasAgentNode& das_client);
};

}  // namespace link_creation_agent