#include "link_creation_service.h"

using namespace link_creation_agent;
using namespace std;
using namespace query_node;

LinkCreationService::LinkCreationService(int thread_count) : thread_pool(thread_count) {}

LinkCreationService::~LinkCreationService() {}

void LinkCreationService::process_request(shared_ptr<RemoteIterator<HandlesAnswer>> iterator,
                                          DasAgentNode* das_client,
                                          vector<string>& link_template,
                                          int max_query_answers) {
    auto job = [this, iterator, das_client, link_template, max_query_answers]() {
        QueryAnswer* query_answer;
        int count = 0;
        long start = time(0);
        while (!iterator->finished()) {
            this_thread::sleep_for(chrono::seconds(1));
            // timeout
            if (time(0) - start > timeout) {
#ifdef DEBUG
                cout << "LinkCreationService::process_request: Timeout for iterator ID: "
                     << iterator->get_local_id() << endl;
#endif
                return;
            }
            if ((query_answer = iterator->pop()) == NULL) {
#ifdef DEBUG
                cout << "LinkCreationService::process_request: Waiting for query answer ID: "
                     << iterator->get_local_id() << endl;
#endif
                Utils::sleep();
            } else {
#ifdef DEBUG
                cout << "LinkCreationService::process_request: Processing query_answer ID: "
                     << iterator->get_local_id() << endl;
#endif
                if (link_template.front() == "LIST") {
                    LinkCreateTemplateList link_create_template_list(link_template);
                    for (auto link_template : link_create_template_list.get_templates()) {
                        Link link(query_answer, link_template.tokenize());
                        this->create_link(link, *das_client);
                    }
                } else {
                    Link link(query_answer, link_template);
                    this->create_link(link, *das_client);
                }
                delete query_answer;
                if (++count == max_query_answers) break;
            }
        }
#ifdef DEBUG
        cout << "LinkCreationService::process_request: Finished processing iterator ID: "
             << iterator->get_local_id() << endl;
#endif
    };

    thread_pool.enqueue(job);
}

void LinkCreationService::create_link(Link& link, DasAgentNode& das_client) {
    // TODO check an alternative to locking this method
    std::unique_lock<std::mutex> lock(m_mutex);
#ifdef DEBUG
    cout << "LinkCreationService::create_link: Creating link" << endl;
#endif
    vector<string> link_tokens = link.tokenize();
    das_client.create_link(link_tokens);
    m_cond.notify_one();
}

void LinkCreationService::set_timeout(int timeout) { this->timeout = timeout; }
