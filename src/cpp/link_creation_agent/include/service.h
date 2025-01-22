/**
 * @file service.h
 * @brief
 */
#pragma once
#include <functional>
#include <atomic>
#include "utils/thread_pool.h"
#include "das_node/query/remote_iterator.h"
#include "das_node/das_server/das_server_node.h"
#include "link.h"
#include <map>
#include <set>

using namespace query_element;
using namespace das;

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
        void process_request(shared_ptr<RemoteIterator> iterator, ServerNode &das_client, string& link_template);
        /**
         * @brief Destructor
         */
        ~LinkCreationService();


    private:
        ThreadPool thread_pool;
        // this can be changed to a better data structure
        set<string> processed_link_handles;

        /**
         * @brief Create a link, blocking the client until the link is created
         * @param link_representation Link object
         * @param das_client DAS Node client
         */
        void create_link(Link link_representation, ServerNode &das_client);
    };

}