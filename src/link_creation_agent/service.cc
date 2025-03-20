#include "service.h"

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
        // TODO timeout
        QueryAnswer* query_answer;
        int count = 0;
        while (!iterator->finished()) {
            this_thread::sleep_for(chrono::seconds(1));
            if ((query_answer = iterator->pop()) == NULL) {
                cout << "LinkCreationService::process_request: Waiting for query answer" << endl;
                Utils::sleep();
            } else {
                cout << "LinkCreationService::process_request: Processing query_answer" << endl;
                if (link_template.front() == "LIST") {
                    LinkCreateTemplateList link_create_template_list(link_template);
                    for(auto link_template : link_create_template_list.get_templates()){
                        Link link(query_answer, link_template.tokenize());
                        this->create_link(link, *das_client);
                    }
                }else{
                    Link link(query_answer, link_template);
                    this->create_link(link, *das_client);
                }
                delete query_answer;
                if (++count == max_query_answers) break;
            }
        }
        cout << "LinkCreationService::process_request: Finished processing iterator" << endl;
    };

    thread_pool.enqueue(job);
}

void LinkCreationService::create_link(Link& link, DasAgentNode& das_client) {
    // TODO check an alternative to locking this method
    std::unique_lock<std::mutex> lock(m_mutex);
    cout << "LinkCreationService::create_link: Creating link" << endl;
    vector<string> link_tokens = link.tokenize();
    das_client.create_link(link_tokens);
    m_cond.notify_one();
}
