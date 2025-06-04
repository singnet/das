#pragma once

#include <grpcpp/grpcpp.h>

#include <cstring>

// clang-format off
#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"
#include "Utils.h"
// clang-format on

#include "And.h"
#include "AtomDBAPITypes.h"
#include "AtomDBSingleton.h"
#include "AttentionBrokerServer.h"
#include "ImportanceList.h"
#include "Iterator.h"
#include "QueryAnswer.h"
#include "QueryElementRegistry.h"
#include "QueryNode.h"
#include "SharedQueue.h"
#include "Source.h"
#include "Terminal.h"
#include "attention_broker.grpc.pb.h"
#include "attention_broker.pb.h"
#include "expression_hasher.h"
#include "thread_pool.h"
#include <shared_mutex>

#define MAX_GET_IMPORTANCE_BUNDLE_SIZE ((unsigned int) 100000)

using namespace std;
using namespace query_engine;
using namespace attention_broker_server;
using namespace atomdb;

namespace query_element {


    class TargetsHandleList : public atomdb_api_types::HandleList {
       public:
       TargetsHandleList(shared_ptr<atomdb_api_types::AtomDocument> document) : atomdb_api_types::HandleList() {
            this->handles_size = document->get_size("targets");
            this->handles = new char*[this->handles_size];
            for (unsigned int i = 0; i < this->handles_size; i++) {
                this->handles[i] = strndup(document->get("targets", i), HANDLE_HASH_SIZE - 1);
            }
            // this->document = document;

        }
        ~TargetsHandleList() {
            for (unsigned int i = 0; i < this->handles_size; i++) {
                free(this->handles[i]);
            }
            delete[] this->handles;
        }

        const char* get_handle(unsigned int index) {
            if (index > this->handles_size) {
                Utils::error("Handle index out of bounds: " + to_string(index) +
                             " Answer handles size: " + to_string(this->handles_size));
            }
            //
            return handles[index];
        }
        
        unsigned int size() { return this->handles_size; }

        private:
        unsigned int handles_size;
        char** handles;
        // shared_ptr<atomdb_api_types::AtomDocument> document;
    };

/**
 * Concrete Source that searches for a pattern in the AtomDB and feeds the QueryElement up in the
 * query tree with the resulting links.
 *
 * A pattern is something like:
 *
 * Similarity
 *    Human
 *    $v1
 *
 * In the example, any links of type "Similarity" pointing to Human as the first target would be
 * returned. These returned links are then fed into the subsequent QueryElement in the tree.
 *
 * LinkTemplate query the AtomDB for the links that match the pattern. In addition to this, it
 * attaches values for any variables in the pattern and sorts all the AtomDB answers by importance
 * (by querying the AttentionBroker) before following up the links (most important ones first).
 *
 * An arbitrary number of nested levels are allowed. For instance:
 *
 * Expression
 *     Symbol A
 *     Symbol B
 *     $v1
 *     Expression
 *         Symbol C
 *         $v2
 *     Expression
 *         $v1
 *         $v2
 *         Expression
 *             Symbol X
 *             Symbol Y
 *             Symbol Z
 *
 * Returned links are guaranteed to satisfy all variable settings properly.
 */
template <unsigned int ARITY>
class LinkTemplate : public Source {
   public:
    // --------------------------------------------------------------------------------------------
    // Constructors and destructors

    /**
     * Constructor expects an array of QueryElements which can be Terminals or nested LinkTemplate.
     *
     * @param type Link type or WILDCARD to indicate that the link type doesn't matter.
     * @param targets An array with targets which can each be a Terminal or a nested LinkTemplate.
     * @param context An optional string defining the context used by the AttentionBroker to
     *        consider STI (short term importance).
     */
    LinkTemplate(const string& type,
                 array<shared_ptr<QueryElement>, ARITY>&& targets,
                 const string& context = "",
                 QueryElementRegistry* query_element_registry = nullptr) {
        this->context = context;
        this->arity = ARITY;
        this->type = type;
        this->query_element_registry = query_element_registry;
        this->target_template = move(targets);
        this->fetch_finished = false;
        this->atom_document = NULL;
        this->local_answers = NULL;
        this->local_answers_size = 0;
        this->local_buffer_processor = NULL;
        bool wildcard_flag = (type == AtomDB::WILDCARD);
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
        this->handle =
            shared_ptr<char>(composite_hash(this->handle_keys, ARITY + 1), default_delete<char[]>());
        if (!wildcard_flag) {
            free(this->handle_keys[0]);
        }
        // This is correct. id is not necessarily a handle but an identifier. It just happens
        // that we want the string for this identifier to be the same as the string representing
        // the handle.
        this->id = this->handle.get() + std::to_string(LinkTemplate::next_instance_count());
        LOG_INFO("LinkTemplate " << this->to_string());
    }

    /**
     * Destructor.
     */
    virtual ~LinkTemplate() {
        this->graceful_shutdown();
        lock_guard<mutex> lock(this->local_answers_mutex);
        if (this->atom_document) delete[] this->atom_document;
        if (this->local_answers) delete[] this->local_answers;
        if (this->next_inner_answer) delete[] this->next_inner_answer;
        while (!this->local_buffer.empty()) {
            delete (QueryAnswer*) this->local_buffer.dequeue();
        }
        for (auto* answer : this->inner_answers) {
            if (answer) delete answer;
        }
        this->inner_answers.clear();
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
        if (this->local_buffer_processor != NULL) {
            this->local_buffer_processor->join();
            delete this->local_buffer_processor;
            this->local_buffer_processor = NULL;
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
        this->local_buffer_processor = new thread(&LinkTemplate::local_buffer_processor_method, this);
        LOG_INFO("Starting local buffer processor thread for link template");
        fetch_links();
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

    struct less_than_query_answer {
        inline bool operator()(const QueryAnswer* qa1, const QueryAnswer* qa2) {
            // Reversed check as we want descending sort
            return (qa1->importance > qa2->importance);
        }
    };

    void increment_local_answers_size() {
        local_answers_mutex.lock();
        this->local_answers_size++;
        local_answers_mutex.unlock();
    }

    unsigned int get_local_answers_size() {
        unsigned int answer;
        local_answers_mutex.lock();
        answer = this->local_answers_size;
        local_answers_mutex.unlock();
        return answer;
    }

    shared_ptr<dasproto::ImportanceList> get_importance(const dasproto::HandleList& handle_list) {
        if (this->query_element_registry != nullptr) {
            LinkTemplate<ARITY>::get_importance_mutex.lock();
            auto il = this->query_element_registry->get(this->handle);
            if (il != nullptr) {
                LinkTemplate<ARITY>::get_importance_mutex.unlock();
                return dynamic_pointer_cast<ImportanceList>(il)->importance_list;
            }
        }
        auto importance_list = make_shared<dasproto::ImportanceList>();
        auto stub = dasproto::AttentionBroker::NewStub(
            grpc::CreateChannel(this->attention_broker_address, grpc::InsecureChannelCredentials()));

        if (handle_list.list_size() <= MAX_GET_IMPORTANCE_BUNDLE_SIZE) {
            stub->get_importance(new grpc::ClientContext(), handle_list, importance_list.get());
        } else {
            LOG_DEBUG("Paginating get_importance()");
            unsigned int page_count = 1;

            dasproto::HandleList small_handle_list;
            dasproto::ImportanceList small_importance_list;
            unsigned int remaining = handle_list.list_size();
            unsigned int cursor = 0;
            while (remaining > 0) {
                LOG_DEBUG("get_importance() page: " << page_count++);
                for (unsigned int i = 0; i < MAX_GET_IMPORTANCE_BUNDLE_SIZE; i++) {
                    if (cursor == handle_list.list_size()) {
                        break;
                    }
                    small_handle_list.add_list(handle_list.list(cursor++));
                    remaining--;
                }
                LOG_DEBUG("get_importance() discharging: " << small_handle_list.list_size());
                stub->get_importance(
                    new grpc::ClientContext(), small_handle_list, &small_importance_list);
                for (unsigned int i = 0; i < small_importance_list.list_size(); i++) {
                    importance_list->add_list(small_importance_list.list(i));
                }
                small_handle_list.clear_list();
                small_importance_list.clear_list();
            }
        }
        if (this->query_element_registry != nullptr) {
            this->query_element_registry->add(this->handle,
                                              make_shared<ImportanceList>(importance_list));
            LinkTemplate<ARITY>::get_importance_mutex.unlock();
        }
        return importance_list;
    }

    void fetch_links() {
        shared_ptr<AtomDB> db = AtomDBSingleton::get_instance();
        this->fetch_result = db->query_for_pattern(this->handle);
        unsigned int answer_count = this->fetch_result->size();
        LOG_INFO("Fetched " << answer_count << " links for link template " << this->to_string());
        QueryAnswer* query_answer;
        vector<QueryAnswer*> fetched_answers;
        fetched_answers.reserve(answer_count);
        if (answer_count > 0) {
            dasproto::HandleList handle_list;
            handle_list.set_context(this->context);
            auto it = this->fetch_result->get_iterator();
            char* handle;
            while ((handle = it->next()) != nullptr) {
                handle_list.add_list(handle);
            }
            auto importance_list = get_importance(handle_list);
            if (importance_list->list_size() != answer_count) {
                Utils::error("Invalid AttentionBroker answer. Size: " +
                             std::to_string(importance_list->list_size()) +
                             " Expected size: " + std::to_string(answer_count));
            }
            this->atom_document = new shared_ptr<atomdb_api_types::HandleList>[answer_count];
            this->local_answers = new QueryAnswer*[answer_count];
            this->next_inner_answer = new unsigned int[answer_count];
            it = this->fetch_result->get_iterator();
            unsigned int ii = 0;
            unsigned int db_count = 0;
            StopWatch stopwatch;
            stopwatch.start();
            shared_mutex t_mutex;
            LOG_INFO("Fetching atom documents");
            int chunk_size = 1000;
            chunk_size = chunk_size > answer_count ? answer_count -1 : chunk_size;
            while ((handle = it->next()) != nullptr) {
                if (((ii > 0) && ((ii % chunk_size) == 0))) {
                    int start = ii - chunk_size;
                    int end = ii;
                    if (end + chunk_size > answer_count) {
                        end = answer_count;
                    }
                    auto job = [this, db, &db_count, start, end, &fetched_answers, importance_list, &t_mutex, &stopwatch, &handle_list]() {
                        vector<string> handles;
                        for (int i = start; i < end; i++) {
                            handles.push_back(handle_list.list(i));
                        }
                        auto atom_handle_list = db->get_atom_documents(handles);
                        int c = 0;
                        for (int i = start; i < end; i++) {
                            auto hh = handle_list.list(i).c_str();
                            this->atom_document[i] = make_shared<TargetsHandleList>(atom_handle_list[c++]);
                            db_count++;
                            // shared_ptr<QueryAnswer> query_answer = make_shared<QueryAnswer>(hh, importance_list->list(i));
                            QueryAnswer* query_answer = new QueryAnswer(strndup(hh, HANDLE_HASH_SIZE - 1), importance_list->list(i));
                            for (unsigned int j = 0; j < this->arity; j++) {
                                if (this->target_template[j]->is_terminal) {
                                    auto terminal = dynamic_pointer_cast<Terminal>(this->target_template[j]);
                                    if (terminal->is_variable) {
                                        if (!query_answer->assignment.assign(
                                                terminal->name.c_str(), this->atom_document[i]->get_handle(j))) {
                                            Utils::error(
                                                "Error assigning variable: " + terminal->name +
                                                " a value: " );
                                        }
                                    }
                                }
                            }
                            // show percentage
                            int total = handle_list.list_size();
                            if (db_count % (total / 10) == 0) {
                                LOG_INFO("Fetched " << (double(db_count) / double(total)) * double(100) << "% atom documents from Mongo DB: " << db_count << "/" << total);
                            }
                            fetched_answers[i] = query_answer;
                        }

                    };
                    thread_pool.enqueue(job);
                }
                ii++;
            }
            // LOG_INFO("Waiting for threads to finish, processing " << ii << " links");
            while (!thread_pool.empty()) {
                // LOG_INFO("Waiting for threads to finish, running threads " << thread_pool.size() << " total of links: " << answer_count );
                Utils::sleep();
            }
            thread_pool.wait();
            stopwatch.stop();
            LOG_INFO("Fetched " << db_count << " from MongoDB of  " << answer_count << " in: " << stopwatch.milliseconds() / 1000 << " seconds");  
            LOG_INFO("Sorting");
            std::sort(fetched_answers.begin(), fetched_answers.end(), less_than_query_answer());
            for (unsigned int i = 0; i < answer_count; i++) {
                if (this->inner_template.size() == 0) {
                    this->local_buffer.enqueue((void*) fetched_answers[i]);
                } else {
                    this->local_answers[i] = fetched_answers[i];
                    this->next_inner_answer[i] = 0;
                    this->increment_local_answers_size();
                }
            }
            LOG_INFO("Sorting answers finished");
            if (this->inner_template.size() == 0) {
                set_flow_finished();
            }
        } else {
            set_flow_finished();
        }
        LOG_INFO("Fetching links finished");
    }

    bool is_feasible(unsigned int index) {
        // LOG_INFO("Checking feasibility for index");
        unsigned int inner_answers_size = inner_answers.size();
        unsigned int cursor = this->next_inner_answer[index];
        while (cursor < inner_answers_size) {
            if (this->inner_answers[cursor] != NULL) {
                bool passed_first_check = true;
                unsigned int arity = this->atom_document[index]->size();
                unsigned int target_cursor = 0;
                for (unsigned int i = 0; i < arity; i++) {
                    // Note to reviewer: pointer comparison is correct here
                    if (this->handle_keys[i + 1] == (char*) AtomDB::WILDCARD.c_str()) {
                        if (target_cursor > this->inner_answers[cursor]->handles_size) {
                            Utils::error("Invalid query answer in inner link template match");
                        }
                        if (strncmp(this->atom_document[index]->get_handle(i),
                                    this->inner_answers[cursor]->handles[target_cursor++],
                                    HANDLE_HASH_SIZE)) {
                            passed_first_check = false;
                            break;
                        }
                    }
                }
                if (passed_first_check &&
                    this->local_answers[index]->merge(this->inner_answers[cursor], false)) {
                    this->inner_answers[cursor] = NULL;
                    return true;
                }
            }
            this->next_inner_answer[index]++;
            cursor++;
        }
        return false;
    }

    bool ingest_newly_arrived_answers() {
        bool flag = false;
        QueryAnswer* query_answer;
        while ((query_answer = dynamic_cast<QueryAnswer*>(this->inner_template_iterator->pop())) !=
               NULL) {
            this->inner_answers.push_back(query_answer);
            flag = true;
        }
        return flag;
    }

    void local_buffer_processor_method() {
        if (this->inner_template.size() == 0) {
            // LOG_INFO("Inner");
            while (!(this->is_flow_finished() && this->local_buffer.empty())) {
                // LOG_INFO("Processing local buffer");
                QueryAnswer* query_answer;
                while ((query_answer = (QueryAnswer*) this->local_buffer.dequeue()) != NULL) {
                    this->output_buffer->add_query_answer(query_answer);
                }
                Utils::sleep();
            }
        } else {
            // LOG_INFO("External");
            while (!this->is_flow_finished()) {
                unsigned int size = get_local_answers_size();
                if (ingest_newly_arrived_answers()) {
                    // LOG_INFO("Processing ingested answers");
                    for (unsigned int i = 0; i < size; i++) {
                        if (this->local_answers[i] != NULL) {
                            if (is_feasible(i)) {
                                this->output_buffer->add_query_answer(this->local_answers[i]);
                                this->local_answers[i] = NULL;
                            } else {
                                if (this->inner_template_iterator->finished()) {
                                    this->local_answers[i] = NULL;
                                }
                            }
                        }
                    }
                } else {
                    // LOG_INFO("Waiting for inner template iterator to finish");
                    if (this->inner_template_iterator->finished()) {
                        // LOG_INFO("Inner template iterator finished");
                        for (unsigned int i = 0; i < size; i++) {
                            if (this->local_answers[i] != NULL) {
                                if (is_feasible(i)) {
                                    this->output_buffer->add_query_answer(this->local_answers[i]);
                                }
                                this->local_answers[i] = NULL;
                            }
                        }
                    } else {
                        Utils::sleep();
                    }
                }
                // LOG_INFO("All answers finshed");  
                bool finished_flag = true;
                for (unsigned int i = 0; i < size; i++) {
                    // LOG_INFO("Checking if answer is finished");
                    if (this->local_answers[i] != NULL) {
                        finished_flag = false;
                        break;
                    }
                }
                if (this->inner_template_iterator->finished()) {
                    // LOG_INFO("Inner template iterator finished");
                    set_flow_finished();
                }
            }
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


    struct AtomDocumentTargets{
        shared_ptr<atomdb_api_types::HandleList> targets;
    };
    string type;
    array<shared_ptr<QueryElement>, ARITY> target_template;
    shared_ptr<char> handle;
    char* handle_keys[ARITY + 1];
    shared_ptr<atomdb_api_types::HandleSet> fetch_result;
    vector<shared_ptr<atomdb_api_types::AtomDocument>> atom_documents;
    vector<shared_ptr<QueryElement>> inner_template;
    SharedQueue local_buffer;
    thread* local_buffer_processor;
    bool fetch_finished;
    mutex fetch_finished_mutex;
    shared_ptr<QueryNodeServer> target_buffer[ARITY];
    shared_ptr<Iterator> inner_template_iterator;
    shared_ptr<atomdb_api_types::HandleList>* atom_document;
    QueryAnswer** local_answers;
    unsigned int* next_inner_answer;
    vector<QueryAnswer*> inner_answers;
    unsigned int local_answers_size;
    mutex local_answers_mutex;
    string context;
    QueryElementRegistry* query_element_registry;
    static mutex get_importance_mutex;
    ThreadPool thread_pool = ThreadPool(16);

};

template <unsigned int ARITY>
std::mutex LinkTemplate<ARITY>::get_importance_mutex;

}  // namespace query_element
