#include "link_creation_service.h"
#include "Logger.h"

#include <fstream>

#include "Logger.h"
#include "link_creation_console.h"

using namespace link_creation_agent;
using namespace std;
using namespace query_node;
using namespace query_engine;

static void add_to_file(string file_path, string file_name, string content) {
    LOG_DEBUG("LinkCreationService::add_to_file: Adding to file: " << file_path << "/" << file_name);
    ofstream file(file_path + "/" + file_name, ios::app);
    if (file.is_open()) {
        file << content << endl;
        file.close();
    } else {
        cerr << "Error opening file: " << file_path + "/" + file_name << endl;
    }
}

LinkCreationService::LinkCreationService(int thread_count, shared_ptr<DASNode> das_node)
    : thread_pool(thread_count) {
    this->link_template_processor = make_shared<LinkTemplateProcessor>();
    this->equivalence_processor = make_shared<EquivalenceProcessor>();
    this->implication_processor = make_shared<ImplicationProcessor>();
    this->equivalence_processor->set_das_node(das_node);
    this->implication_processor->set_das_node(das_node);
    // run create_link in a separate thread
    this->create_link_thread = thread(&LinkCreationService::create_link_threaded, this);
}

void LinkCreationService::set_query_agent_mutex(shared_ptr<mutex> query_agent_mutex) {
    this->query_agent_mutex = query_agent_mutex;
    this->implication_processor->set_mutex(query_agent_mutex);
    this->equivalence_processor->set_mutex(query_agent_mutex);
}

LinkCreationService::~LinkCreationService() {
    is_stoping = true;
    if (create_link_thread.joinable()) {
        create_link_thread.join();
    }
}

void LinkCreationService::process_request(shared_ptr<RemoteIterator<HandlesAnswer>> iterator,
                                          DasAgentNode* das_client,
                                          vector<string>& link_template,
                                          const string& context,
                                          const string& request_id,
                                          int max_query_answers) {
    if (this->das_client == nullptr) {
        this->das_client = das_client;
    }
    auto job = [this, iterator, das_client, link_template, max_query_answers, context, request_id]() {
        LOG_DEBUG("LinkCreationService::process_request: " << request_id << "Processing request ID: " << request_id);
        QueryAnswer* query_answer;
        int count = 0;
        long start = time(0);
        while (!iterator->finished() || this->is_stoping) {
            // this_thread::sleep_for(chrono::seconds(1));
            if (time(0) - start > timeout) {
                LOG_DEBUG("LinkCreationService::process_request: " << request_id <<"Timeout for iterator ID: "
                          << iterator->get_local_id());
                return;
            }
            if ((query_answer = iterator->pop()) == NULL) {
                // LOG_DEBUG("LinkCreationService::process_request: No query answer for iterator ID: "
                //           << iterator->get_local_id());
                Utils::sleep();
            } else {
                LOG_DEBUG("LinkCreationService::process_request:" << request_id << "Processing query_answer ID: "
                          << iterator->get_local_id());

                try {
                    vector<vector<string>> link_tokens;
                    vector<string> extra_params;
                    extra_params.push_back(context);
                    if (LinkCreationProcessor::get_processor_type(link_template.front()) ==
                        ProcessorType::PROOF_OF_IMPLICATION) {
                        link_tokens = implication_processor->process(query_answer, extra_params);
                    } else if (LinkCreationProcessor::get_processor_type(link_template.front()) ==
                               ProcessorType::PROOF_OF_EQUIVALENCE) {
                        link_tokens = equivalence_processor->process(query_answer, extra_params);
                    } else {
                        link_tokens = link_template_processor->process(query_answer, link_template);
                    }
                    for (auto& link : link_tokens) {
                        this->link_creation_queue.enqueue(make_tuple(request_id, link));
                    }
                    // unique_lock<mutex> lock(this->m_mutex);
                    // this->create_link(link_tokens, *das_client, iterator->get_local_id());
                    // lock.unlock();
                    delete query_answer;
                } catch (const std::exception& e) {
                    LOG_ERROR("LinkCreationService::process_request: " << request_id << "Exception: " << e.what());
                    delete query_answer;
                    continue;
                }
                if (++count == max_query_answers) break;
            }
        }
        LOG_DEBUG("LinkCreationService::process_request: " << request_id << "Finished processing iterator ID: " + iterator->get_local_id());
    };

    thread_pool.enqueue(job);
}

void LinkCreationService::create_link(std::vector<std::vector<std::string>>& links,
                                      DasAgentNode& das_client,
                                      string id) {
    // TODO check an alternative to locking this method
    try {
        for (vector<string> link_tokens : links) {
            string metta_str = link_creation_agent::Console::get_instance()->print_metta(link_tokens);
            add_to_file(metta_file_path, id + ".metta", metta_str);
            das_client.create_link(link_tokens);
        }
    } catch (const std::exception& e) {
        LOG_ERROR("LinkCreationService::create_link: Exception: " << e.what());
    }
}

void LinkCreationService::set_timeout(int timeout) { this->timeout = timeout; }

void LinkCreationService::create_link_threaded() {
    while (!is_stoping) {
        if (!link_creation_queue.empty()) {
            LOG_DEBUG("LinkCreationService::create_link: Creating link " << 5);
            auto request_map = link_creation_queue.dequeue();
            string id = get<0>(request_map);
            vector<string> request = get<1>(request_map);
            string meta_content = link_creation_agent::Console::get_instance()->print_metta(request);
            add_to_file(metta_file_path, id + ".metta", meta_content);
            das_client->create_link(request);
        }
        this_thread::sleep_for(chrono::milliseconds(300));
    }
}

void LinkCreationService::set_metta_file_path(string metta_file_path) {
    this->metta_file_path = metta_file_path;
}