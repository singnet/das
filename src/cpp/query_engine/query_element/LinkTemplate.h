#ifndef _QUERY_ELEMENT_LINKTEMPLATE_H
#define _QUERY_ELEMENT_LINKTEMPLATE_H

#include <cstring>

#include "QueryNode.h"
#include "Source.h"
#include "Iterator.h"
#include "And.h"
#include "Terminal.h"
#include "AtomDBSingleton.h"
#include "AtomDBAPITypes.h"
#include "QueryAnswer.h"
#include "expression_hasher.h"
#include "SharedQueue.h"

#include "AttentionBrokerServer.h"
#include "attention_broker.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include "attention_broker.pb.h"

#define MAX_GET_IMPORTANCE_BUNDLE_SIZE ((unsigned int) 100000)

using namespace std;
using namespace query_engine;
using namespace attention_broker_server;

namespace query_element {

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
    LinkTemplate(
        const string &type, 
        const array<QueryElement *, ARITY> &targets, 
        const string &context = "") {

        this->context = context;
        this->arity = ARITY;
        this->type = type;
        this->target_template = targets;
        this->fetch_finished = false;
        this->atom_document = NULL;
        this->local_answers = NULL;
        this->local_answers_size = 0;
        this->local_buffer_processor = NULL;
        bool wildcard_flag = (type == AtomDB::WILDCARD);
        this->handle_keys[0] = (wildcard_flag ?
            (char *) AtomDB::WILDCARD.c_str() :
            named_type_hash((char *) type.c_str()));
        for (unsigned int i = 1; i <= ARITY; i++) {
            // It's safe to get stored shared_ptr's raw pointer here because handle_keys[]
            // is used solely in this scope so it's guaranteed that handle will not be freed.
            if (targets[i - 1]->is_terminal) {
                this->handle_keys[i] = ((Terminal *) targets[i - 1])->handle.get();
            } else {
                this->handle_keys[i] = (char *) AtomDB::WILDCARD.c_str();
                this->inner_template.push_back(targets[i - 1]);
            }
        }
        this->handle = shared_ptr<char>(composite_hash(this->handle_keys, ARITY + 1));
        if (! wildcard_flag) {
            free(this->handle_keys[0]);
        }
        // This is correct. id is not necessarily a handle but an identifier. It just happens
        // that we want the string for this identifier to be the same as the string representing
        // the handle.
        this->id = this->handle.get() + std::to_string(LinkTemplate::next_instance_count());
    }

    /**
     * Destructor.
     */
    virtual ~LinkTemplate() {
#ifdef DEBUG
        cout << "LinkTemplate::LinkTemplate() DESTRUCTOR BEGIN" << endl;
#endif
        graceful_shutdown();
        local_answers_mutex.lock();
        if (local_answers_size > 0) {
            delete [] this->atom_document;
            delete [] this->local_answers;
            delete [] this->next_inner_answer;
        }
        local_answers_mutex.unlock();
#ifdef DEBUG
        cout << "LinkTemplate::LinkTemplate() DESTRUCTOR END" << endl;
#endif
    }

    // --------------------------------------------------------------------------------------------
    // QueryElement API

    /**
     * Gracefully shuts down this QueryElement's processor thread.
     */
    virtual void graceful_shutdown() {
#ifdef DEBUG
        cout << "LinkTemplate::graceful_shutdown() BEGIN" << endl;
#endif
        set_flow_finished();
        if (this->local_buffer_processor != NULL) {
            this->local_buffer_processor->join();
            this->local_buffer_processor = NULL;
        }
        Source::graceful_shutdown();
#ifdef DEBUG
        cout << "LinkTemplate::graceful_shutdown() END" << endl;
#endif
    }

    virtual void setup_buffers() {
#ifdef DEBUG
        cout << "LinkTemplate::setup_buffers() BEGIN" << endl;
#endif
        Source::setup_buffers();
        if (this->inner_template.size() > 0) {
            switch(this->inner_template.size()) {
                case 1: {
                    this->inner_template_iterator = shared_ptr<Iterator>(new Iterator(
                        inner_template[0]
                    ));
                    break;
                }
                case 2: {
                    this->inner_template_iterator = shared_ptr<Iterator>(new Iterator(
                        new And<2>({
                            inner_template[0],
                            inner_template[1]
                        }),
                        true
                    ));
                    break;
                }
                case 3: {
                    this->inner_template_iterator = shared_ptr<Iterator>(new Iterator(
                        new And<3>({
                            inner_template[0],
                            inner_template[1],
                            inner_template[2]
                        }),
                        true
                    ));
                    break;
                }
                case 4: {
                    this->inner_template_iterator = shared_ptr<Iterator>(new Iterator(
                        new And<4>({
                            inner_template[0],
                            inner_template[1],
                            inner_template[2],
                            inner_template[3]
                        }),
                        true
                    ));
                    break;
                }
                default: {
                    Utils::error("Invalid number of inner templates (> 4) in link template.");
                }
            }
        }
        this->local_buffer_processor = new thread(
            &LinkTemplate::local_buffer_processor_method, 
            this);
        fetch_links();
#ifdef DEBUG
        cout << "LinkTemplate::setup_buffers() END" << endl;
#endif
    }

private:

    struct less_than_query_answer {
        inline bool operator() (const QueryAnswer *qa1, const QueryAnswer *qa2) {
            // Reversed check as we want descending sort
            return (qa1->importance > qa2->importance);
        }
    };

    // --------------------------------------------------------------------------------------------
    // Private methods

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

    void get_importance(
        const dasproto::HandleList &handle_list, 
        dasproto::ImportanceList &importance_list) {

        auto stub = dasproto::AttentionBroker::NewStub(grpc::CreateChannel(
            this->attention_broker_address, 
            grpc::InsecureChannelCredentials()));

        if (handle_list.list_size() <= MAX_GET_IMPORTANCE_BUNDLE_SIZE) {
            stub->get_importance(new grpc::ClientContext(), handle_list, &importance_list);
            return;
        }

#ifdef DEBUG
        cout << "get_importance() paginating" << endl;
        unsigned int page_count = 1;
#endif

        dasproto::HandleList small_handle_list;
        dasproto::ImportanceList small_importance_list;
        unsigned int remaining = handle_list.list_size();
        unsigned int cursor = 0;
        while (remaining > 0) {
#ifdef DEBUG
        cout << "get_importance() page: " << page_count++ << endl;
#endif
            for (unsigned int i = 0; i < MAX_GET_IMPORTANCE_BUNDLE_SIZE; i++) {
                if (cursor == handle_list.list_size()) {
                    break;
                }
                small_handle_list.add_list(handle_list.list(cursor++));
                remaining--;
            }
#ifdef DEBUG
        cout << "discharging: " << small_handle_list.list_size() << endl;
#endif
            stub->get_importance(new grpc::ClientContext(), small_handle_list, &small_importance_list);
            for (unsigned int i = 0; i < small_importance_list.list_size(); i++) {
                importance_list.add_list(small_importance_list.list(i));
            }
            small_handle_list.clear_list();
            small_importance_list.clear_list();
        }
    }

    void fetch_links() {
#ifdef DEBUG
        cout << "fetch_links() BEGIN" << endl;
        cout << "fetch_links() Pattern handle: " << this->handle << endl;
#endif
        shared_ptr<AtomDB> db = AtomDBSingleton::get_instance();
        this->fetch_result = db->query_for_pattern(this->handle);
        unsigned int answer_count = this->fetch_result->size();
#ifdef DEBUG
        cout << "fetch_links() ac: " << answer_count << endl;
#endif
        QueryAnswer *query_answer;
        vector<QueryAnswer *> fetched_answers;
        if (answer_count > 0) {
            dasproto::HandleList handle_list;
            handle_list.set_context(this->context);
            for (unsigned int i = 0; i < answer_count; i++) {
                handle_list.add_list(this->fetch_result->get_handle(i));
            }
            dasproto::ImportanceList importance_list;
            get_importance(handle_list, importance_list);
            if (importance_list.list_size() != answer_count) {
                Utils::error("Invalid AttentionBroker answer. Size: " + 
                    std::to_string(importance_list.list_size()) +
                    " Expected size: " + std::to_string(answer_count));
            }
            this->atom_document = new shared_ptr<atomdb_api_types::AtomDocument>[answer_count];
            this->local_answers = new QueryAnswer *[answer_count];
            this->next_inner_answer = new unsigned int[answer_count];
            for (unsigned int i = 0; i < answer_count; i++) {
                this->atom_document[i] = db->get_atom_document(this->fetch_result->get_handle(i));
                query_answer = new QueryAnswer(this->fetch_result->get_handle(i), importance_list.list(i));
                const char *s = this->atom_document[i]->get("targets", 0);
                for (unsigned int j = 0; j < this->arity; j++) {
                    if (this->target_template[j]->is_terminal) {
                        Terminal *terminal = (Terminal *) this->target_template[j];
                        if (terminal->is_variable) {
                            if (! query_answer->assignment.assign(
                                    terminal->name.c_str(),
                                    this->atom_document[i]->get("targets", j))) {
                                Utils::error(
                                    "Error assigning variable: " +
                                    terminal->name +
                                    " a value: " +
                                    string(this->atom_document[i]->get("targets", j)));
                            }
                        }
                    }
                }
                fetched_answers.push_back(query_answer);
            }
            std::sort(fetched_answers.begin(), fetched_answers.end(), less_than_query_answer());
            for (unsigned int i = 0; i < answer_count; i++) {
                if (this->inner_template.size() == 0) {
                    this->local_buffer.enqueue((void *) fetched_answers[i]);
                } else {
                    this->local_answers[i] = fetched_answers[i];
                    this->next_inner_answer[i] = 0;
                    this->increment_local_answers_size();
                }
            }
            if (this->inner_template.size() == 0) {
                set_flow_finished();
            }
        } else {
            set_flow_finished();
        }
#ifdef DEBUG
        cout << "fetch_links() END" << endl;
#endif
    }

    bool is_feasible(unsigned int index) {
        unsigned int inner_answers_size = inner_answers.size();
        unsigned int cursor = this->next_inner_answer[index];
        while (cursor < inner_answers_size) {
            if (this->inner_answers[cursor] != NULL) {
                bool passed_first_check = true;
                unsigned int arity = this->atom_document[index]->get_size("targets");
                unsigned int target_cursor = 0;
                for (unsigned int i = 0; i < arity; i++) {
                    // Note to reviewer: pointer comparison is correct here
                    if (this->handle_keys[i + 1] == (char *) AtomDB::WILDCARD.c_str()) {
                        if (target_cursor > this->inner_answers[cursor]->handles_size) {
                            Utils::error("Invalid query answer in inner link template match");
                        }
                        if (strncmp(
                            this->atom_document[index]->get("targets", i), 
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
        QueryAnswer *query_answer;
        while ((query_answer = this->inner_template_iterator->pop()) != NULL) {
            this->inner_answers.push_back(query_answer);
            flag = true;
        }
        return flag;
    }

    void local_buffer_processor_method() {
        if (this->inner_template.size() == 0) {
            while (! (this->is_flow_finished() && this->local_buffer.empty())) {
                QueryAnswer *query_answer;
                while ((query_answer = (QueryAnswer *) this->local_buffer.dequeue()) != NULL) {
                    this->output_buffer->add_query_answer(query_answer);
                }
                Utils::sleep();
            }
        } else {
            while (! this->is_flow_finished()) {
                unsigned int size = get_local_answers_size();
                if (ingest_newly_arrived_answers()) {
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
                    if (this->inner_template_iterator->finished()) {
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
                bool finished_flag = true;
                for (unsigned int i = 0; i < size; i++) {
                    if (this->local_answers[i] != NULL) {
                        finished_flag = false;
                        break;
                    }
                }
                if (this->inner_template_iterator->finished()) {
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

private:

    string type;
    array<QueryElement *, ARITY> target_template;
    unsigned int arity;
    shared_ptr<char> handle;
    char *handle_keys[ARITY + 1];
    shared_ptr<atomdb_api_types::HandleList> fetch_result;
    vector<shared_ptr<atomdb_api_types::AtomDocument>> atom_documents;
    vector<QueryElement *> inner_template;
    SharedQueue local_buffer;
    thread *local_buffer_processor;
    bool fetch_finished;
    mutex fetch_finished_mutex;
    shared_ptr<QueryNodeServer> target_buffer[ARITY];
    shared_ptr<Iterator> inner_template_iterator;
    shared_ptr<atomdb_api_types::AtomDocument> *atom_document;
    QueryAnswer **local_answers;
    unsigned int *next_inner_answer;
    vector<QueryAnswer *> inner_answers;
    unsigned int local_answers_size;
    mutex local_answers_mutex;
    string context;
};

} // namespace query_element

#endif // _QUERY_ELEMENT_LINKTEMPLATE_H
