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
#include "LinkTemplate.h"
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
        this->keys_template[0] = this->handle_keys[0];  // to be used in the `get_link_handle` method
        for (unsigned int i = 1; i <= ARITY; i++) {
            this->keys_template[i] = NULL;
            if (this->target_template[i - 1]->is_operator) {
                Utils::error("LinkTemplate2 does not support operators.");
            }
            // It's safe to get stored shared_ptr's raw pointer here because handle_keys[]
            // is used solely in this scope so it's guaranteed that handle will not be freed.
            if (this->target_template[i - 1]->is_terminal) {
                auto terminal = dynamic_pointer_cast<Terminal>(this->target_template[i - 1]);
                if (terminal->is_variable) {
                    Utils::error("LinkTemplate2 does not support variables.");
                }
                this->handle_keys[i] =
                    dynamic_pointer_cast<Terminal>(this->target_template[i - 1])->handle.get();
                this->keys_template[i] =
                    this->handle_keys[i];  // to be used in the `get_link_handle` method
            } else {
                this->handle_keys[i] = (char*) AtomDB::WILDCARD.c_str();
                this->inner_template.push_back(this->target_template[i - 1]);
                this->inner_positions.push_back(i);  // to be used in the `get_link_handle` method
            }
        }

        this->validate_inner_template();

        this->handle =
            shared_ptr<char>(composite_hash(this->handle_keys, ARITY + 1), default_delete<char[]>());

        // moved to the destructor
        /*if (!wildcard_flag) {
            free(this->handle_keys[0]);
        }*/

        // This is correct. id is not necessarily a handle but an identifier. It just happens
        // that we want the string for this identifier to be the same as the string representing
        // the handle.
        this->id = this->handle.get() + std::to_string(LinkTemplate2::next_instance_count());
        LOG_INFO("LinkTemplate2 " << this->to_string());
    }

    void validate_inner_template() {
        if (this->inner_template.size() == 0) {
            Utils::error("LinkTemplate2: No inner template.");
        }
        vector<pair<size_t, string>> variables;
        bool is_lt2 = false;
        for (auto& inner_template : this->inner_template) {
            is_lt2 = false;
            variables = {};
            switch (inner_template->arity) {
                case 1:
                    if (auto lt = dynamic_pointer_cast<LinkTemplate<1>>(inner_template)) {
                        variables = lt->get_variables();
                    } else if (auto lt2 = dynamic_pointer_cast<LinkTemplate2<1>>(inner_template)) {
                        is_lt2 = true;
                    }
                    break;
                case 2:
                    if (auto lt = dynamic_pointer_cast<LinkTemplate<2>>(inner_template)) {
                        variables = lt->get_variables();
                    } else if (auto lt2 = dynamic_pointer_cast<LinkTemplate2<2>>(inner_template)) {
                        is_lt2 = true;
                    }
                    break;
                case 3:
                    if (auto lt = dynamic_pointer_cast<LinkTemplate<3>>(inner_template)) {
                        variables = lt->get_variables();
                    } else if (auto lt2 = dynamic_pointer_cast<LinkTemplate2<3>>(inner_template)) {
                        is_lt2 = true;
                    }
                    break;
                case 4:
                    if (auto lt = dynamic_pointer_cast<LinkTemplate<4>>(inner_template)) {
                        variables = lt->get_variables();
                    } else if (auto lt2 = dynamic_pointer_cast<LinkTemplate2<4>>(inner_template)) {
                        is_lt2 = true;
                    }
                    break;
                default:
                    Utils::error("Invalid number of inner templates in LinkTemplate2.");
            }
            if (is_lt2) {
                Utils::error("LinkTemplate2 does not support LinkTemplate2 as inner templates.");
            }
            if (variables.size() == 0) {
                Utils::error("LinkTemplate2 does not support inner templates without variables.");
            }
            this->inner_template_variables.push_back(variables);
        }
    }

    /**
     * Destructor.
     */
    virtual ~LinkTemplate2() {
        this->graceful_shutdown();
        if (this->handle_keys[0] != (char*) AtomDB::WILDCARD.c_str()) {
            free(this->handle_keys[0]);
        }
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

    vector<pair<size_t, string>> get_variables() {
        vector<pair<size_t, string>> variables;
        for (unsigned int i = 0; i < this->arity; i++) {
            if (this->target_template[i]->is_terminal) {
                auto terminal = dynamic_pointer_cast<Terminal>(this->target_template[i]);
                if (terminal->is_variable) {
                    variables.push_back({i, terminal->name});
                }
            }
        }
        return variables;
    }

   private:
    // --------------------------------------------------------------------------------------------
    // Private methods and attributes

    /**
     * Generates a link handle by matching data from QueryAnswer with the LinkTemplate2 structure.
     * This function iterates over the inner templates, extracting variables and identifying matches
     * between the template's targets and the QueryAnswer's assignment. If a match is found, it
     * assigns the query answer's handle to the template's key. The resulting composite hash of keys
     * is returned as the link handle. If no match is found for a template, the function returns NULL.
     *
     * @note This function is not thread-safe.
     * @note Caller is responsible for freeing the returned char pointer.
     *
     * @param query_answer Pointer to a QueryAnswer object containing handles and variable assignments.
     * @return A char pointer representing the composite hash link handle, or NULL if no match exists.
     */
    char* get_link_handle(QueryAnswer* query_answer) {
        vector<pair<size_t, string>> variables;
        size_t inner_template_index = 0;
        shared_ptr<atomdb::atomdb_api_types::AtomDocument> atom;
        for (auto inner_position : this->inner_positions) {
            this->keys_template[inner_position] = NULL;
            if (inner_template_index >= this->inner_template.size()) {
                Utils::error("Invalid inner template index.");
            }
            variables = this->inner_template_variables[inner_template_index++];
            // clang-format off
            for (
                size_t qa_handles_index = 0;
                qa_handles_index < query_answer->handles_size;
                qa_handles_index++
            ) {
                // clang-format on
                auto qa_handle = query_answer->handles[qa_handles_index];
                atom = this->db->get_atom_document(qa_handle);
                if (!atom->contains("targets")) continue;
                for (auto& [index, name] : variables) {
                    auto target_handle = atom->get("targets", index);
                    if (!target_handle) continue;
                    auto assignment_handle = query_answer->assignment.get(name.c_str());
                    if (!assignment_handle) continue;
                    if (strcmp(target_handle, assignment_handle) == 0) {
                        this->keys_template[inner_position] = (char*) qa_handle;
                        break;
                    }
                }
                if (this->keys_template[inner_position] != NULL) break;
            }
            if (this->keys_template[inner_position] == NULL) return NULL;
        }
        return composite_hash(this->keys_template, ARITY + 1);
    }

    void inner_templates_processor_method() {
        while (!this->is_flow_finished()) {
            if (this->inner_template_iterator->finished()) {
                this->set_flow_finished();
                break;
            }
            QueryAnswer* query_answer;
            this->link_handles.clear();
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
                this->link_handles.push_back(link_handle);
                this->link_handles_to_query_answers[link_handle] = query_answer;
                free(link_handle);
                if (this->link_handles.size() >= DB_LINK_HANDLES_BATCH_SIZE) break;
            }
            if (this->link_handles.size() == 0) continue;
            auto existing_handles = this->db->links_exist(this->link_handles);
            for (auto& link_handle : existing_handles) {
                query_answer = this->link_handles_to_query_answers[link_handle];
                query_answer->handles[0] = strdup(link_handle.c_str());
                query_answer->handles_size = 1;
                this->output_buffer->add_query_answer(query_answer);
                this->link_handles_to_query_answers.erase(link_handle);
            }
            for (auto& [_, query_answer] : this->link_handles_to_query_answers) delete query_answer;
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
    shared_ptr<char> handle;
    char* handle_keys[ARITY + 1];
    char* keys_template[ARITY + 1];  // to be used in the `get_link_handle` method
    vector<size_t> inner_positions;  // to be used in the `get_link_handle` method
    vector<shared_ptr<QueryElement>> inner_template;
    vector<vector<pair<size_t, string>>> inner_template_variables;
    thread* inner_templates_processor;
    shared_ptr<Iterator> inner_template_iterator;
    shared_ptr<AtomDB> db;
    vector<string> link_handles;
    unordered_map<string, QueryAnswer*> link_handles_to_query_answers;
};

}  // namespace query_element
