#include "link_creation_service.h"

#include <fstream>

#include "Logger.h"
#include "Utils.h"
#include "link_creation_console.h"

using namespace link_creation_agent;
using namespace std;
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

LinkCreationService::LinkCreationService(int thread_count) : thread_pool(thread_count) {
    this->link_template_processor = make_shared<LinkTemplateProcessor>();
    this->equivalence_processor = make_shared<EquivalenceProcessor>();
    this->implication_processor = make_shared<ImplicationProcessor>();
    // run create_link in a separate thread
    this->create_link_thread = thread(&LinkCreationService::create_links, this);
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
                } catch (const exception& e) {
                    LOG_ERROR("[" << request_id << "]"
                                  << " Exception: " << e.what());
                    continue;
                }
                if (++count == max_query_answers) break;
            }
            if (count == max_query_answers) break;
        }
        Utils::sleep(1000);
        LOG_INFO("[" << request_id << "]"
                     << " - Finished processing iterator ID: " + proxy->my_id()
                     << " with count: " << count);
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

void LinkCreationService::set_timeout(int timeout) { this->timeout = timeout; }

void LinkCreationService::create_links() {
    while (!is_stoping) {
        if (!link_creation_queue.empty()) {
            auto request_map = link_creation_queue.dequeue();
            string id = get<0>(request_map);
            vector<string> request = get<1>(request_map);
            try {
                string meta_content =
                    link_creation_agent::Console::get_instance()->tokens_to_metta_string(request);
                if (meta_content.empty()) {
                    LOG_ERROR("Failed to create MeTTa expression for " << Utils::join(request, ' '));
                    continue;
                }
                if (metta_expression_set.find(meta_content) != metta_expression_set.end()) {
                    LOG_INFO("Duplicate link creation request, skipping.");
                    continue;
                }
                if (this->save_links_to_metta_file) {
                    LOG_INFO("MeTTa Expression: " << meta_content);
                    add_to_file(metta_file_path, id + ".metta", meta_content);
                }
                if (this->save_links_to_db) {
                    LOG_INFO("TOKENS: " << Utils::join(request, ' '));
                    das_client->create_link(request);
                }
                metta_expression_set.insert(meta_content);
            } catch (const exception& e) {
                LOG_ERROR("Exception: " << e.what());
            }
        } else {
            this_thread::sleep_for(chrono::milliseconds(300));
        }
    }
}

void LinkCreationService::set_metta_file_path(string metta_file_path) {
    this->metta_file_path = metta_file_path;
}