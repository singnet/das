#include "link_creation_service.h"

#include <fstream>

#include "Logger.h"
#include "Utils.h"
#include "link_creation_console.h"

using namespace link_creation_agent;
using namespace std;
// using namespace query_node;
using namespace query_engine;

static void add_to_file(string file_path, string file_name, string content) {
    LOG_INFO("Saving to file: " << file_path << "/" << file_name);
    ofstream file(file_path + "/" + file_name, ios::app);
    if (file.is_open()) {
        file << content << endl;
        file.close();
    } else {
        LOG_ERROR("Error opening file: " << file_path + "/" + file_name);
    }
}

LinkCreationService::LinkCreationService(int thread_count, shared_ptr<service_bus::ServiceBus> das_node)
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

void LinkCreationService::enqueue_link_creation_request(const string& request_id,
                                                        const vector<vector<string>>& link_tokens) {
    for (const auto& link : link_tokens) {
        link_creation_queue.enqueue(make_tuple(request_id, link));
    }
}

void LinkCreationService::process_request(shared_ptr<PatternMatchingQueryProxy> proxy,
                                          DasAgentNode* das_client,
                                          vector<string>& link_template,
                                          const string& context,
                                          const string& request_id,
                                          int max_query_answers) {
    if (this->das_client == nullptr) {
        this->das_client = das_client;
    }
    auto job = [this, proxy, das_client, link_template, max_query_answers, context, request_id]() {
        LOG_INFO("[" << request_id << "] - Processing request");
        shared_ptr<QueryAnswer> query_answer;
        int count = 0;
        long start = time(0);
        while (!proxy->finished()) {
            if (time(0) - start > this->timeout) {
                LOG_INFO("[" << request_id << "]"
                             << " - Timeout for iterator ID: " << proxy->my_id());
                return;
            }
            if ((query_answer = proxy->pop()) == NULL) {
                Utils::sleep();
            } else {
                try {
                    vector<vector<string>> link_tokens;
                    vector<string> extra_params;
                    extra_params.push_back(context);
                    shared_lock lock(m_mutex);
                    link_tokens = process_query_answer(query_answer, extra_params, link_template);
                    enqueue_link_creation_request(request_id, link_tokens);
                } catch (const std::exception& e) {
                    LOG_ERROR("[" << request_id << "]"
                                  << " Exception: " << e.what());
                    continue;
                }
                if (++count == max_query_answers) break;
            }
            if (count == max_query_answers) break;
        }
        LOG_INFO("[" << request_id << "]"
                     << " - Finished processing iterator ID: " + proxy->my_id());
    };

    thread_pool.enqueue(job);
}

vector<vector<string>> LinkCreationService::process_query_answer(shared_ptr<QueryAnswer> query_answer,
                                                                 vector<string> params,
                                                                 vector<string> link_template) {
    if (LinkCreationProcessor::get_processor_type(link_template.front()) ==
        ProcessorType::PROOF_OF_IMPLICATION) {
        return implication_processor->process(query_answer, params);
    } else if (LinkCreationProcessor::get_processor_type(link_template.front()) ==
               ProcessorType::PROOF_OF_EQUIVALENCE) {
        return equivalence_processor->process(query_answer, params);
    } else {
        return link_template_processor->process(query_answer, link_template);
    }
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
        LOG_ERROR("Exception: " << e.what());
    }
}

void LinkCreationService::set_timeout(int timeout) { this->timeout = timeout; }

void LinkCreationService::create_link_threaded() {
    while (!is_stoping) {
        if (!link_creation_queue.empty()) {
            auto request_map = link_creation_queue.dequeue();
            string id = get<0>(request_map);
            vector<string> request = get<1>(request_map);
            string meta_content = link_creation_agent::Console::get_instance()->print_metta(request);
            if (metta_expression_set.find(meta_content) != metta_expression_set.end()) {
                LOG_INFO("Duplicate link creation request, skipping.");
                continue;
            }
            try {
                add_to_file(metta_file_path, id + ".metta", meta_content);
                das_client->create_link(request);
            } catch (const std::exception& e) {
                LOG_ERROR("Exception: " << e.what());
            }
            metta_expression_set.insert(meta_content);
        } else {
            this_thread::sleep_for(chrono::milliseconds(300));
        }
    }
}

void LinkCreationService::set_metta_file_path(string metta_file_path) {
    this->metta_file_path = metta_file_path;
}