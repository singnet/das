/**
 * @file service.h
 * @brief
 */
#pragma once
#include <functional>
#include <atomic>
#include "thread_pool.h"
#include "remote_iterator.h"
#include "das_server_node.h"

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
        void process_request(RemoteIterator &iterator, ServerNode &das_client, string& link_template);
        /**
         * @brief Destructor
         */
        ~LinkCreationService();


    private:
        ThreadPool thread_pool;
    };

}