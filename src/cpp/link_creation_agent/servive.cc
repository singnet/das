#include "service.h"

using namespace link_creation_agent;
using namespace std;
using namespace query_node;

LinkCreationService::LinkCreationService(int thread_count) : thread_pool(thread_count)
{
}

LinkCreationService::~LinkCreationService()
{
}

void LinkCreationService::process_request(shared_ptr<RemoteIterator> iterator, ServerNode *das_client, vector<string> &link_template)
{
    auto job = [this, iterator, das_client, link_template]() {
        while (!iterator->finished())
        {
            QueryAnswer* query_answer = iterator->pop();
            Link link(*query_answer, link_template);
            this->create_link(link, *das_client);
            delete query_answer;
        }
    };
    thread_pool.enqueue(job);
}

void LinkCreationService::create_link(Link &link, ServerNode &das_client)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    cout << "Creating link" << endl;
    vector<string> link_tokens = link.tokenize();
    das_client.create_link(link_tokens);
    m_cond.notify_one();
}
