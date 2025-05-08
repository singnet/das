#pragma once

#include <cstring>

// clang-format off
#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"
// clang-format on

#include "And.h"
#include "AtomDBAPITypes.h"
#include "AtomDBSingleton.h"
#include "Iterator.h"
#include "QueryAnswer.h"
#include "QueryNode.h"
#include "SharedQueue.h"
#include "Source.h"
#include "Terminal.h"
#include "expression_hasher.h"

using namespace std;
using namespace query_engine;
using namespace atomdb;

#define DB_LINK_HANDLES_BATCH_SIZE ((unsigned int) 1000)

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
                            make_shared<And<2>>(
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

    void inner_templates_processor_method() {
        while (!this->is_flow_finished()) {
            if (this->inner_template_iterator->finished()) {
                this->set_flow_finished();
                break;
            }
            QueryAnswer* query_answer;
            this->link_handles_count = 0;
            this->link_handles_to_query_answers.clear();
            // clang-format off
            while (
                (query_answer = 
                    dynamic_cast<QueryAnswer*>(this->inner_template_iterator->pop())) != NULL) {
                // clang-format on
                auto link_handle = this->get_link_handle(query_answer);
                if (link_handle == NULL) {
                    delete query_answer;
                    continue;
                }
                this->link_handles[this->link_handles_count++] = link_handle;
                this->link_handles_to_query_answers[link_handle] = query_answer;
                if (this->link_handles_count == DB_LINK_HANDLES_BATCH_SIZE) {
                    break;
                }
            }
            if (this->link_handles_count == 0) {
                continue;
            }
            size_t existing_handles_count =
                this->db->links_exist(this->link_handles, this->link_handles_count);
            char* link_handle;
            for (size_t i = 0; i < existing_handles_count; i++) {
                link_handle = this->link_handles[i];
                query_answer = this->link_handles_to_query_answers[link_handle];
                query_answer->handles[0] = link_handle;
                query_answer->handles_size = 1;
                this->output_buffer->add_query_answer(query_answer);
                this->link_handles_to_query_answers.erase(link_handle);
                this->link_handles[i] = NULL;
            }
            for (auto& [_, query_answer] : this->link_handles_to_query_answers) {
                delete query_answer;
            }
            for (size_t i = 0; i < this->link_handles_count; i++) {
                if (this->link_handles[i] != NULL) {
                    free(this->link_handles[i]);
                    this->link_handles[i] = NULL;
                }
            }
            this->link_handles_count = 0;
            this->link_handles_to_query_answers.clear();
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
    char* link_handles[DB_LINK_HANDLES_BATCH_SIZE];
    size_t link_handles_count;
    unordered_map<string, QueryAnswer*> link_handles_to_query_answers;
};

}  // namespace query_element
