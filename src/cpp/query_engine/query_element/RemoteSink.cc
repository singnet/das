#include <cstring>
#include <stack>
#include <unordered_map>
#include "RemoteSink.h"
#include "AtomDBSingleton.h"
#include "AtomDBAPITypes.h"

#include "AttentionBrokerServer.h"
#include "attention_broker.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include "attention_broker.pb.h"

#define MAX_CORRELATIONS_WITHOUT_STIMULATE ((unsigned int) 1000)
#define MAX_STIMULATE_COUNT ((unsigned int) 1)

using namespace query_element;

string RemoteSink::DEFAULT_ATTENTION_BROKER_PORT = "37007";

// -------------------------------------------------------------------------------------------------
// Constructors and destructors

RemoteSink::RemoteSink(
    QueryElement *precedent, 
    const string &local_id,
    const string &remote_id,
    bool update_attention_broker_flag,
    const string &context,
    bool delete_precedent_on_destructor) : 
    Sink(precedent, "RemoteSink(" + precedent->id + ")", delete_precedent_on_destructor, false) {
#ifdef DEBUG
    cout << "RemoteSink::RemoteSink() BEGIN" << endl;
    cout << "RemoteSink::RemoteSink() local_id: " << local_id << endl;
    cout << "RemoteSink::RemoteSink() remote_id: " << remote_id << endl;
#endif

    this->attention_broker_address = "localhost:" + RemoteSink::DEFAULT_ATTENTION_BROKER_PORT;
    this->query_context = context;
    this->local_id = local_id;
    this->remote_id = remote_id;
    this->queue_processor = NULL;
    this->attention_broker_postprocess = NULL;
    this->update_attention_broker_flag = update_attention_broker_flag;
    RemoteSink::setup_buffers();
    Sink::setup_buffers();
    this->queue_processor = new thread(&RemoteSink::queue_processor_method, this);
    if (this->update_attention_broker_flag) {
        this->attention_broker_postprocess = new \
            thread(&RemoteSink::attention_broker_postprocess_method, this);
    }
#ifdef DEBUG
    cout << "RemoteSink::RemoteSink() END" << endl;
#endif
}

RemoteSink::~RemoteSink() {
    graceful_shutdown();
}

// -------------------------------------------------------------------------------------------------
// Public methods

void RemoteSink::setup_buffers() {
#ifdef DEBUG
    cout << "RemoteSink::setup_buffers() BEGIN" << endl;
#endif
    this->remote_output_buffer = shared_ptr<QueryNode>(new QueryNodeClient(
        this->local_id, 
        this->remote_id, 
        MessageBrokerType::GRPC));
#ifdef DEBUG
    cout << "RemoteSink::setup_buffers() END" << endl;
#endif
}

void RemoteSink::graceful_shutdown() {
#ifdef DEBUG
    cout << "RemoteSink::graceful_shutdown() BEGIN" << endl;
#endif
    Sink::graceful_shutdown();
    set_flow_finished();
    set_attention_broker_postprocess_finished();
    if (this->queue_processor != NULL) {
        this->queue_processor->join();
    }
    if (this->attention_broker_postprocess != NULL) {
        this->attention_broker_postprocess->join();
    }
    this->remote_output_buffer->graceful_shutdown();
#ifdef DEBUG
    cout << "RemoteSink::graceful_shutdown() END" << endl;
#endif
}

// -------------------------------------------------------------------------------------------------
// Private methods

void RemoteSink::queue_processor_method() {
#ifdef DEBUG
    cout << "RemoteSink::queue_processor_method() BEGIN" << endl;
#endif
    do {
        if (is_flow_finished() || 
           (this->input_buffer->is_query_answers_finished() && 
            this->input_buffer->is_query_answers_empty())) {

            break;
        }
        bool idle_flag = true;
        QueryAnswer *query_answer;
        while ((query_answer = this->input_buffer->pop_query_answer()) != NULL) {
            this->remote_output_buffer->add_query_answer(query_answer);
            if (this->update_attention_broker_flag) {
                this->attention_broker_queue.enqueue((void *) query_answer);
                //update_attention_broker((QueryAnswer *) query_answer);
            }
            idle_flag = false;
        }
        if (idle_flag) {
            Utils::sleep();
        }
    } while (true);
#ifdef DEBUG
    cout << "RemoteSink::queue_processor_method() ready to return" << endl;
#endif
    this->remote_output_buffer->query_answers_finished();
    set_flow_finished();
    set_attention_broker_postprocess_finished();
#ifdef DEBUG
    cout << "RemoteSink::queue_processor_method() END" << endl;
#endif
}

/*
static bool visit_function(HandleTrie::TrieNode *node, void *data) {
    ((unordered_map<string, unsigned int> *) data)->insert({
        node->suffix,
        ((AccumulatorValue *) node->value)->count
        });
    return false;
}
*/

void RemoteSink::attention_broker_postprocess_method() {

    // GRPC stuff

    // TODO: XXX Review allocation performance in all this method
    set<string> single_answer; // Auxiliary set of handles.
    unordered_map<string, unsigned int> joint_answer; // Auxiliary joint count of handles.
    //HandleTrie *joint_answer = new HandleTrie(HANDLE_HASH_SIZE - 1);

    // Protobuf data structures
    dasproto::HandleList *handle_list; // will contain single_answer (can't be used directly
                                      // because it's a list, not a set.
    dasproto::HandleCount handle_count; // Counting of how many times each handle appeared
                                        // in all single_entry
    dasproto::Ack *ack; // Command return

    shared_ptr<AtomDB> db = AtomDBSingleton::get_instance();
    shared_ptr<atomdb_api_types::HandleList> query_result;
    stack<string> execution_stack;
    unsigned int weight_sum;
    unsigned int correlated_count = 0;
    unsigned int stimulated_count = 0;

#ifdef DEBUG
    unsigned int count_total_processed = 0;
#endif

    //handle_list.set_context(this->query_context);
    do {
        if (is_attention_broker_postprocess_finished() ||
            (is_flow_finished() && this->attention_broker_queue.empty())) {
            break;
        }
        bool idle_flag = true;
        QueryAnswer *query_answer;
        string handle;
        unsigned int count;
        while ((query_answer = (QueryAnswer *) this->attention_broker_queue.dequeue()) != NULL) {
            if (stimulated_count == MAX_STIMULATE_COUNT) {
                continue;
            }
#ifdef DEBUG
            count_total_processed++;
            if ((count_total_processed % 1000) == 0) {
                cout << "RemoteSink::attention_broker_postprocess_method() count_total_processed: " << count_total_processed << endl;
            }
#endif
            for (unsigned int i = 0; i < query_answer->handles_size; i++) {
                execution_stack.push(string(query_answer->handles[i]));
            }
            while (! execution_stack.empty()) {
                handle = execution_stack.top();
                execution_stack.pop();
                // Updates single_answer (correlation)
                single_answer.insert(handle);
                // Updates joint answer (stimulation)
                if (joint_answer.find(handle) != joint_answer.end()) {
                    count = joint_answer[handle] + 1;
                } else {
                    count = 1;
                }
                joint_answer[handle] = count;
                //joint_answer->insert(handle, new AccumulatorValue());
                // Gets targets and stack them
                query_result = db->query_for_targets((char *) handle.c_str());
                if (query_result != NULL) { // if handle is link
                    unsigned int query_result_size = query_result->size();
                    for (unsigned int i = 0; i < query_result_size; i++) {
                        execution_stack.push(string(query_result->get_handle(i)));
                    }
                }
            }
            //handle_list.mutable_list()->Clear();
            //handle_list.clear_list();
            auto stub = dasproto::AttentionBroker::NewStub(grpc::CreateChannel(
                this->attention_broker_address,
                grpc::InsecureChannelCredentials())); // XXXXX Move this up
            handle_list = new dasproto::HandleList();
            handle_list->set_context(this->query_context);
            for (auto handle_it: single_answer) {
                handle_list->add_list(handle_it);
            }
            single_answer.clear();
            ack = new dasproto::Ack();
#ifdef DEBUG
            //cout << "RemoteSink::attention_broker_postprocess_method() requesting CORRELATE" << endl;
#endif
            stub->correlate(new grpc::ClientContext(), *handle_list, ack);
            if (ack->msg() != "CORRELATE") {
                Utils::error("Failed GRPC command: AttentionBroker::correlate()");
            }
            idle_flag = false;
            if (++correlated_count == MAX_CORRELATIONS_WITHOUT_STIMULATE) {
                correlated_count = 0;
                for (auto const& pair: joint_answer) {
                    (*handle_count.mutable_map())[pair.first] = pair.second;
                    weight_sum += pair.second;
                }
                (*handle_count.mutable_map())["SUM"] = weight_sum;
                ack = new dasproto::Ack();
                auto stub = dasproto::AttentionBroker::NewStub(grpc::CreateChannel(
                    this->attention_broker_address,
                    grpc::InsecureChannelCredentials()));
#ifdef DEBUG
                cout << "RemoteSink::attention_broker_postprocess_method() requesting STIMULATE" << endl;
#endif
                handle_count.set_context(this->query_context);
                stub->stimulate(new grpc::ClientContext(), handle_count, ack);
                stimulated_count++;
                if (ack->msg() != "STIMULATE") {
                    Utils::error("Failed GRPC command: AttentionBroker::stimulate()");
                }
                joint_answer.clear();
            }
        }
        if (idle_flag) {
            Utils::sleep();
        }
    } while (true);
    //joint_answer->traverse(true, &visit_function, &joint_answer_map);
    if (correlated_count > 0) {
        weight_sum = 0;
        for (auto const& pair: joint_answer) {
            (*handle_count.mutable_map())[pair.first] = pair.second;
            weight_sum += pair.second;
        }
        (*handle_count.mutable_map())["SUM"] = weight_sum;
        ack = new dasproto::Ack();
        auto stub = dasproto::AttentionBroker::NewStub(grpc::CreateChannel(
            this->attention_broker_address,
            grpc::InsecureChannelCredentials()));
#ifdef DEBUG
        cout << "RemoteSink::attention_broker_postprocess_method() requesting STIMULATE" << endl;
#endif
        handle_count.set_context(this->query_context);
        stub->stimulate(new grpc::ClientContext(), handle_count, ack);
        stimulated_count++;
        if (ack->msg() != "STIMULATE") {
            Utils::error("Failed GRPC command: AttentionBroker::stimulate()");
        }
    }
    //delete joint_answer;
    set_attention_broker_postprocess_finished();
}

void RemoteSink::set_attention_broker_postprocess_finished() {
    this->attention_broker_postprocess_finished_mutex.lock();
    this->attention_broker_postprocess_finished = true;
    this->attention_broker_postprocess_finished_mutex.unlock();
}

bool RemoteSink::is_attention_broker_postprocess_finished() {
    bool answer;
    this->attention_broker_postprocess_finished_mutex.lock();
    answer = this->attention_broker_postprocess_finished;
    this->attention_broker_postprocess_finished_mutex.unlock();
    return answer;
}

