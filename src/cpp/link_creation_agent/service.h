/**
 * @file service.h
 * @brief
 */
#pragma once
#include <functional>
#include <atomic>
#include "thread_pool.h"
// #include "remote_iterator.h"
#include "RemoteIterator.h"
#include "das_server_node.h"
#include "link.h"
#include <map>
#include <set>
#include <mutex>
#include <condition_variable>

using namespace das;
using namespace query_node;
using namespace query_element;
namespace link_creation_agent
{
    class LinkCreationService
    {

    public:
        LinkCreationService(int thread_count);
        /**
         * @brief Add an iterator to process in thread pool
         * @param iterator RemoteIterator object
         * @param das_client DAS Node client
         */
        void process_request(shared_ptr<RemoteIterator> iterator, ServerNode *das_client, vector<string>& link_template);
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

        /**
         * @brief Create a link, blocking the client until the link is created
         * @param link Link object
         * @param das_client DAS Node client
         */
        void create_link(Link &link, ServerNode &das_client);
    };

}