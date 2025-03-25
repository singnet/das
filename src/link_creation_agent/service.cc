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
                vector<vector<string>> link_tokens;
                if (LinkCreationProcessor::get_processor_type(link_template.front()) == 
                ProcessorType::PROOF_OF_IMPLICATION) {
                    link_tokens = implication_processor.process(query_answer, link_template);
                }else if (LinkCreationProcessor::get_processor_type(link_template.front()) ==
                ProcessorType::PROOF_OF_EQUIVALENCE) {
                    link_tokens = equivalence_processor.process(query_answer, link_template);
                }else {
                    link_tokens = link_template_processor.process(query_answer, link_template);
                }
                this->create_link(link_tokens, *das_client);
                delete query_answer;
                if (++count == max_query_answers) break;
            }
        }
        cout << "LinkCreationService::process_request: Finished processing iterator" << endl;
    };

    thread_pool.enqueue(job);
}

void LinkCreationService::create_link(std::vector<std::vector<std::string>>& links, DasAgentNode& das_client) {
    // TODO check an alternative to locking this method
    std::unique_lock<std::mutex> lock(m_mutex);
    for (vector<string> link_tokens : links) {
        cout << "LinkCreationService::create_link: Creating link" << endl;
        das_client.create_link(link_tokens);
    }
    m_cond.notify_one();
}
