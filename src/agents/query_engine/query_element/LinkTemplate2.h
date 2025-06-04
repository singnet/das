#pragma once

#include <cstring>
// #include <fstream>
#include <fstream>

// clang-format off
#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"
// clang-format on

#include "And.h"
#include "And2.h"
#include "AtomDBAPITypes.h"
#include "AtomDBSingleton.h"
#include "Iterator.h"
#include "QueryAnswer.h"
#include "QueryNode.h"
#include "SharedQueue.h"
#include "Source.h"
#include "Terminal.h"
#include "expression_hasher.h"
#include "ThreadPool.h"


using namespace std;
using namespace query_engine;
using namespace atomdb;

#define DB_LINK_HANDLES_BATCH_SIZE ((unsigned int) 2000)

namespace query_element {

template <unsigned int ARITY>
class LinkTemplate2 : public Source {
   public:
    // --------------------------------------------------------------------------------------------
    // Constructors and destructors

    LinkTemplate2(const string& type, array<shared_ptr<QueryElement>, ARITY>&& targets) {
        this->arity = ARITY;
        this->type = type;
        this->target_template = move(targets);
        this->inner_templates_processor = NULL;
        bool wildcard_flag = (type == AtomDB::WILDCARD);
        this->db = AtomDBSingleton::get_instance();
        this->handle_keys[0] =
            (wildcard_flag ? (char*) AtomDB::WILDCARD.c_str() : named_type_hash((char*) type.c_str()));
        for (unsigned int i = 1; i <= ARITY; i++) {
            // It's safe to get stored shared_ptr's raw pointer here because handle_keys[]
            // is used solely in this scope so it's guaranteed that handle will not be freed.
            if (this->target_template[i - 1]->is_terminal) {
                this->handle_keys[i] =
                    dynamic_pointer_cast<Terminal>(this->target_template[i - 1])->handle.get();
            } else {
                this->handle_keys[i] = (char*) AtomDB::WILDCARD.c_str();
                this->inner_template.push_back(this->target_template[i - 1]);
            }
        }
        if (this->inner_template.size() == 0) {
            Utils::error("LinkTemplate2: No inner template.");
        }
        this->handle =
            shared_ptr<char>(composite_hash(this->handle_keys, ARITY + 1), default_delete<char[]>());
        if (!wildcard_flag) {
            free(this->handle_keys[0]);
        }
        // This is correct. id is not necessarily a handle but an identifier. It just happens
        // that we want the string for this identifier to be the same as the string representing
        // the handle.
        this->id = this->handle.get() + std::to_string(LinkTemplate2::next_instance_count());
        LOG_INFO("LinkTemplate2 " << this->to_string());
    }

    /**
     * Destructor.
     */
    virtual ~LinkTemplate2() {
        this->graceful_shutdown();
        this->inner_template.clear();
        this->inner_template_iterator.reset();
    }

    // --------------------------------------------------------------------------------------------
    // QueryElement API

    /**
     * Gracefully shuts down this QueryElement's processor thread.
     */
    virtual void graceful_shutdown() {
        if (this->is_flow_finished()) return;
        set_flow_finished();
        if (this->inner_templates_processor != NULL) {
            this->inner_templates_processor->join();
            delete this->inner_templates_processor;
            this->inner_templates_processor = NULL;
        }
        Source::graceful_shutdown();
    }

    virtual void setup_buffers() {
        Source::setup_buffers();
        if (this->inner_template.size() > 0) {
            // clang-format off
            switch (this->inner_template.size()) {
                case 1: {
                    this->inner_template_iterator =
                        make_shared<Iterator>(
                            inner_template[0]
                        );
                    break;
                }
                case 2: {
                    this->inner_template_iterator = 
                        make_shared<Iterator>(
                            make_shared<And2<2>>(
                                array<shared_ptr<QueryElement>, 2>(
                                    {
                                        inner_template[0],
                                        inner_template[1]
                                    }
                                )
                            )
                        );
                    break;
                }
                case 3: {
                    this->inner_template_iterator = 
                        make_shared<Iterator>(
                            make_shared<And<3>>(
                                array<shared_ptr<QueryElement>, 3>(
                                    {
                                        inner_template[0],
                                        inner_template[1],
                                        inner_template[2]
                                    }
                                )
                            )
                        );
                    break;
                }
                case 4: {
                    this->inner_template_iterator =
                        make_shared<Iterator>(
                            make_shared<And<4>>(
                                array<shared_ptr<QueryElement>, 4>(
                                    {
                                        inner_template[0],
                                        inner_template[1],
                                        inner_template[2],
                                        inner_template[3]
                                    }
                                )
                            )
                        );
                    break;
                }
                default: {
                    Utils::error("Invalid number of inner templates (> 4) in link template.");
                }
            }
            // clang-format on
        }
        this->inner_templates_processor =
            new thread(&LinkTemplate2::inner_templates_processor_method, this);
    }

   private:
    // --------------------------------------------------------------------------------------------
    // Private methods and attributes

    char* get_link_handle(QueryAnswer* query_answer) {
        char* handle_keys[ARITY];
        handle_keys[0] = named_type_hash((char*) this->type.c_str());
        auto qa_handles_size = query_answer->handles_size;
        size_t qa_handles_index = 0;
        for (unsigned int i = 0; i < ARITY; i++) {
            if (this->target_template[i]->is_terminal) {
                handle_keys[i + 1] =
                    dynamic_pointer_cast<Terminal>(this->target_template[i])->handle.get();
            } else {
                // QueryAnswer doesn't have one or more handles for this template
                if (qa_handles_index == qa_handles_size) {
                    free(handle_keys[0]);
                    return NULL;
                }
                handle_keys[i + 1] = (char*) query_answer->handles[qa_handles_index++];
            }
        }
        auto hash = composite_hash(handle_keys, ARITY + 1);
        free(handle_keys[0]);
        return hash;
    }

    void save_to_file(const string& filename, unsigned long long id) {
        remove(filename.c_str());
        ofstream file(filename);
        if (file.is_open()) {
            file << "last_processed=" << id << endl;
            file.close();
        } else {
            LOG_ERROR("Unable to open file " << filename);
        }
    }



    void inner_templates_processor_method() {
        unsigned long long count = 0;
        unsigned long long count_qa = 0;
        unsigned long long thread_count = 0;
        // long long last_processed = 113892017;
        // string last_processed_file = "/opt/das/src/bin/last_processed.txt";
        // unsigned long long last_processed = load_from_file(last_processed_file);
        // string last_processed_file = "/opt/das/src/bin/last_processed.txt";
        // auto config_file = Utils::parse_config(last_processed_file);
        // unsigned long long last_processed_start = stoull(config_file["last_processed"]);
        // unsigned long long last_processed = last_processed_start;
        bool skip = false;
        while (!this->is_flow_finished()) {
            if (this->inner_template_iterator->finished()) {
                this->set_flow_finished();
                break;
            }
            QueryAnswer* query_answer;
            // this->link_handles.clear();
            // this->link_handles_to_query_answers.clear();

            auto handle_to_query_answer = new unordered_map<string, QueryAnswer*>();
            auto handles = new vector<string>();
            // LOG_INFO("Waiting AND..."); 
            // Utils::sleep(1000*60*60);
            // clang-format off
            while (
                (query_answer = 
                    dynamic_cast<QueryAnswer*>(this->inner_template_iterator->input_buffer->pop_query_answer())) != NULL) {
                // clang-format on


                // if(count < last_processed){
                //     if(count % 1000000 == 0) {
                //         LOG_INFO("Skiped: "  << count << "/" << last_processed);
                //     }
                //     if (!skip) {
                //         LOG_INFO("Skiping " << last_processed );
                //     }
                //     skip = true;
                //     delete query_answer;
                //     // delete handle_to_query_answer;
                //     // delete handles;
                //     // free(link_handle);
                //     count++;
                //     continue;
                // }
                
                auto link_handle = this->get_link_handle(query_answer);
                if (link_handle == NULL) {
                    delete query_answer;
                    continue;
                }
                count++;
                if(count % 1000 == 0) {
                    LOG_INFO("Processed: " << thread_count << "/" << count << " query answers: " << count_qa);
                    LOG_INFO("Memory usage: " << Utils::memory_usage() / 1024 / 1024 << " GB");
                    // save_to_file(last_processed_file, last_processed_start + thread_count);
                }
                handles->push_back(link_handle);
                (*handle_to_query_answer)[link_handle] = query_answer;
                free(link_handle);
                if (handles->size() >= DB_LINK_HANDLES_BATCH_SIZE) {
                    break;
                }
            }
            if (handles->size() == 0 && !this->inner_template_iterator->finished()) {
                delete handle_to_query_answer;
                delete handles;
                Utils::sleep();
                continue;
            }

            // auto ptrstr1 = handles;
            // auto ptrstr2 = handle_to_query_answer;

            auto handles_shared = std::shared_ptr<std::vector<std::string>>(handles);
            auto handle_to_query_answer_shared = std::shared_ptr<std::unordered_map<std::string, QueryAnswer*>>(handle_to_query_answer);

            auto job = [this,  handles_shared, &count_qa, handle_to_query_answer_shared, &thread_count](){
                // auto handles = ptrstr1;
                // auto handle_to_query_answer = ptrstr2;
                // LOG_INFO("Processing " << handles->size() << " handles");
                auto existing_handles = this->db->get_existing_ids(*handles_shared);
                for (auto handle : existing_handles) {
                    count_qa++;
                    auto query_answer = QueryAnswer::copy((*handle_to_query_answer_shared)[handle]);
                    query_answer->handles[0] = strndup(handle.c_str(), HANDLE_HASH_SIZE - 1);
                    query_answer->handles_size = 1;
                    this->output_buffer->add_query_answer(query_answer);
                }
                for (auto& [link_handle, query_answer] : *handle_to_query_answer_shared) {
                    if (query_answer != NULL) {
                        delete query_answer;
                        query_answer = NULL;
                        thread_count++;
                    }
                }
                handle_to_query_answer_shared->clear();
                handles_shared->clear();
                // handles_shared.reset();
                // handle_to_query_answer_shared.reset();

                // if (handles != NULL) {
                //     delete handles;
                //     handles = NULL;
                // }
                // if (handle_to_query_answer != NULL) {
                //     delete handle_to_query_answer;
                //     handle_to_query_answer = NULL;
                // }

            };

            thread_pool.enqueue(job);
        }
        this->output_buffer->query_answers_finished();
    }

    static unsigned int next_instance_count() {
        static unsigned int instance_count = 0;
        return instance_count++;
    }

    string to_string() {
        string answer = string(this->handle.get()) + " [" + this->type + " <";
        for (unsigned int i = 0; i < this->arity; i++) {
            answer += this->handle_keys[i + 1];  // index 0 must be skipped
            if (this->target_template[i]->id != "") {
                answer += " (" + target_template[i]->id + ")";
            }
            if (i != (this->arity - 1)) {
                answer += ", ";
            }
        }
        answer += ">]";
        return answer;
    }

    string type;
    array<shared_ptr<QueryElement>, ARITY> target_template;
    unsigned int arity;
    shared_ptr<char> handle;
    char* handle_keys[ARITY + 1];
    vector<shared_ptr<QueryElement>> inner_template;
    thread* inner_templates_processor;
    shared_ptr<Iterator> inner_template_iterator;
    shared_ptr<AtomDB> db;
    vector<string> link_handles;
    unordered_map<string, QueryAnswer*> link_handles_to_query_answers;
    ThreadPool thread_pool = ThreadPool(16);
};

}  // namespace query_element
