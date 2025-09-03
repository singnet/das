#include "LinkCreationService.h"

#include <fstream>

#include "AtomDBSingleton.h"
#include "Logger.h"
#include "Utils.h"
#include "expression_hasher.h"

using namespace link_creation_agent;
using namespace std;
using namespace query_engine;
using namespace atomdb;

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
    this->answer_cache = new HandleTrie(ANSWER_CACHE_KEY_SIZE);
    // run create_link in a separate thread
    this->create_link_thread = thread(&LinkCreationService::create_links, this);
}

LinkCreationService::~LinkCreationService() {
    is_stoping = true;
    if (create_link_thread.joinable()) {
        create_link_thread.join();
    }
    delete answer_cache;
}

void LinkCreationService::save_cache() {
    ofstream file(metta_file_path + "/" + "cache");
    if (file.is_open()) {
        // NOTE to Reviewer: leaving this code commented out for now, not sure if it's a bug or 
        // I'm not using correctly, but this approach only saves one key and not all the keys.
        //
        // answer_cache->traverse(
        //     false,
        //     [](HandleTrie::TrieNode* node, void* data) {
        //         ofstream* file = static_cast<ofstream*>(data);
        //         if (node->value != NULL) {
        //             *file << node->suffix << ": ";
        //             LOG_DEBUG("Saving cache entry: " << node->suffix);
        //             ProcessorTypeValue* types = static_cast<ProcessorTypeValue*>(node->value);
        //             for (const auto& type : types->processor_types) {
        //                 *file << static_cast<int>(type) << " ";
        //             }
        //             *file << endl;
        //         }
        //         return true;
        //     },
        //     &file);
        for (const auto& key : answer_cache_keys) {
            file << key << ": ";
            ProcessorTypeValue* types = dynamic_cast<ProcessorTypeValue*>(answer_cache->lookup(key));
            if (types) {
                for (const auto& type : types->processor_types) {
                    file << static_cast<int>(type) << " ";
                }
            }
            file << endl;
        }
        file.close();
    } else {
        LOG_ERROR("Error opening file: " << metta_file_path + "/" + "cache");
    }
}

void LinkCreationService::load_cache() {
    ifstream file(this->metta_file_path + "/" + "cache");
    if (file.is_open()) {
        string line;
        while (getline(file, line)) {
            istringstream iss(line);
            string key;
            if (!(std::getline(iss, key, ':'))) {
                break;
            }
            ProcessorTypeValue* types = new ProcessorTypeValue();
            int type;
            while (iss >> type) {
                types->processor_types.insert(static_cast<ProcessorType>(type));
            }
            answer_cache->insert(key, types);
            answer_cache_keys.push_back(key);
            LOG_DEBUG("Cache entry added: " << key << ": ");
            for (auto& type : types->processor_types) {
                LOG_DEBUG(" - " << static_cast<int>(type));
            }
        }
        file.close();
    } else {
        LOG_ERROR("Error opening file: " << this->metta_file_path + "/" + "cache");
    }
}

string LinkCreationService::query_answer_hash(shared_ptr<QueryAnswer> query_answer) {
    vector<string> variables;
    for (auto& var : query_answer->assignment.table) {
        variables.push_back(var.second);
    }
    auto key = Utils::join(variables, ',');
    return compute_hash((char*) key.c_str());
}

bool LinkCreationService::is_cached(shared_ptr<QueryAnswer> query_answer, ProcessorType type) {
    std::string key = query_answer_hash(query_answer);
    auto it = dynamic_cast<ProcessorTypeValue*>(this->answer_cache->lookup(key));
    if (it == NULL) {
        return false;
    }
    return find(it->processor_types.begin(), it->processor_types.end(), type) !=
           it->processor_types.end();
}

void LinkCreationService::set_cache(shared_ptr<QueryAnswer> query_answer, ProcessorType type) {
    std::string key = query_answer_hash(query_answer);
    auto it = dynamic_cast<ProcessorTypeValue*>(this->answer_cache->lookup(key));
    if (it == NULL) {
        LOG_DEBUG("Cache miss: " << key);
        auto p_value = new ProcessorTypeValue();
        p_value->processor_types.insert(type);
        this->answer_cache->insert(key, p_value);
        answer_cache_keys.push_back(key);
        answer_cache_size++;
    } else {
        it->processor_types.insert(type);
    }
}

void LinkCreationService::process_request(shared_ptr<PatternMatchingQueryProxy> proxy,
                                          shared_ptr<LinkCreationAgentRequest> request) {
    auto link_template = request->link_template;
    auto context = request->context;
    auto request_id = request->id;
    auto max_query_answers = request->max_results;

    auto job = [this, proxy, link_template, max_query_answers, context, request_id, request]() {
        shared_ptr<QueryAnswer> query_answer;
        long start = time(0);
        int cached_count = 0;
        int count = 0;

        while (true) {
            if (time(0) - start > this->timeout) {
                LOG_INFO("[" << request_id << "]"
                             << " - Timeout for iterator ID: " << proxy->my_id());
                return;
            }
            if (request->aborting) {
                LOG_INFO("[" << request_id << "]"
                             << " - Aborting processing for iterator ID: " << proxy->my_id());
                return;
            }
            while ((query_answer = proxy->pop()) != NULL) {
                try {
                    if (request->aborting) {
                        LOG_INFO("[" << request_id << "]"
                                     << " - Aborting processing for iterator ID: " << proxy->my_id());
                        return;
                    }
                    vector<vector<string>> link_tokens;
                    vector<string> extra_params;
                    extra_params.push_back(context);
                    shared_lock lock(m_mutex);
                    auto links =
                        process_query_answer(query_answer, extra_params, link_template, cached_count);
                    save_cache();
                    for (const auto& link : links) {
                        link_creation_queue.enqueue(make_tuple(request_id, link));
                    }
                    // enqueue_link_creation_request(request_id, link_tokens);
                } catch (const exception& e) {
                    LOG_ERROR("[" << request_id << "]"
                                  << " Exception: " << e.what());
                    continue;
                }
                LOG_DEBUG("Processed " << count << " of max: " << max_query_answers
                                       << " cached: " << cached_count);
                if ((++count - cached_count) >= max_query_answers) break;
            }
            if (count >= max_query_answers) break;
            if (proxy->finished()) break;
            Utils::sleep();
        }
        // Utils::sleep(500);
        LOG_INFO("[" << request_id << "]"
                     << " - Finished processing iterator ID: " + proxy->my_id()
                     << " with count: " << count);
        request->is_running = false;
        request->processed += (count);
    };

    thread_pool.enqueue(job);
    request->is_running = true;
}

vector<shared_ptr<Link>> LinkCreationService::process_query_answer(shared_ptr<QueryAnswer> query_answer,
                                                                   vector<string> params,
                                                                   vector<string> link_template,
                                                                   int& cached_count) {
    if (LinkCreationProcessor::get_processor_type(link_template.front()) ==
        ProcessorType::PROOF_OF_IMPLICATION) {
        if (is_cached(query_answer, ProcessorType::PROOF_OF_IMPLICATION)) {
            LOG_DEBUG("Skipping cached query answer: " << query_answer->tokenize());
            cached_count++;
            return {};
        }
        set_cache(query_answer, ProcessorType::PROOF_OF_IMPLICATION);
        return implication_processor->process_query(query_answer, params);
    } else if (LinkCreationProcessor::get_processor_type(link_template.front()) ==
               ProcessorType::PROOF_OF_EQUIVALENCE) {
        if (is_cached(query_answer, ProcessorType::PROOF_OF_EQUIVALENCE)) {
            LOG_DEBUG("Skipping cached query answer: " << query_answer->tokenize());
            cached_count++;
            return {};
        }
        set_cache(query_answer, ProcessorType::PROOF_OF_EQUIVALENCE);
        return equivalence_processor->process_query(query_answer, params);
    } else {
        if (is_cached(query_answer, ProcessorType::TEMPLATE)) {
            LOG_DEBUG("Skipping cached query answer: " << query_answer->tokenize());
            cached_count++;
            return {};
        }
        set_cache(query_answer, ProcessorType::TEMPLATE);
        return link_template_processor->process_query(query_answer, link_template);
    }
}

void LinkCreationService::set_timeout(int timeout) { this->timeout = timeout; }

void LinkCreationService::create_links() {
    while (!is_stoping) {
        if (!link_creation_queue.empty()) {
            auto request_map = link_creation_queue.dequeue();
            string id = get<0>(request_map);
            shared_ptr<Link> link = get<1>(request_map);
            try {
                LOG_INFO("Processing link: " << link->to_string());
                string meta_content = link->metta_representation(*AtomDBSingleton::get_instance());
                if (meta_content.empty()) {
                    LOG_ERROR("Failed to create MeTTa expression for " << link->to_string());
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
                    LOG_INFO("Saving link to database: " << link->to_string());
                    auto db_instance = AtomDBSingleton::get_instance();
                    if (!db_instance->link_exists(link->handle())) {
                        LOG_INFO("Adding link to AtomDB: " << link->to_string());
                        db_instance->add_link(link.get());
                    }
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
    load_cache();
}
