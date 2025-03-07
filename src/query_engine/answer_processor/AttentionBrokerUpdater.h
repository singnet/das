#pragma once

#include <grpcpp/grpcpp.h>

#include <cstring>
#include <stack>
#include <unordered_map>

#include "AtomDBAPITypes.h"
#include "AtomDBSingleton.h"
#include "AttentionBrokerServer.h"
#include "HandlesAnswer.h"
#include "QueryAnswer.h"
#include "QueryAnswerProcessor.h"
#include "SharedQueue.h"
#include "Utils.h"
#include "attention_broker.grpc.pb.h"
#include "attention_broker.pb.h"

#define MAX_CORRELATIONS_WITHOUT_STIMULATE ((unsigned int) 1000)
#define MAX_STIMULATE_COUNT ((unsigned int) 1)

using namespace std;

namespace query_engine {

constexpr char* ATTENTION_BROKER_ADDRESS = "localhost:37007";

class AttentionBrokerUpdater : public QueryAnswerProcessor {
   public:
    AttentionBrokerUpdater(const string& query_context = "")
        : attention_broker_address(ATTENTION_BROKER_ADDRESS),
          flow_finished(false),
          query_context(query_context) {
        string attention_broker_address = Utils::get_environment("DAS_ATTENTION_BROKER_ADDRESS");
        string attention_broker_port = Utils::get_environment("DAS_ATTENTION_BROKER_PORT");
        if (!attention_broker_address.empty() && !attention_broker_port.empty()) {
            this->attention_broker_address = attention_broker_address + ":" + attention_broker_port;
        }
#ifdef DEBUG
        cout << "AttentionBrokerUpdater::AttentionBrokerUpdater() attention_broker_address: "
             << this->attention_broker_address << endl;
#endif
        this->queue_processor_method = new thread(&AttentionBrokerUpdater::queue_processor, this);
    }
    virtual ~AttentionBrokerUpdater() { this->graceful_shutdown(); };
    virtual void process_answer(QueryAnswer* query_answer) override {
        this->answers_queue.enqueue((void*) query_answer);
    }
    virtual void query_answers_finished() override { this->set_flow_finished(); }
    virtual void graceful_shutdown() override {
        this->set_flow_finished();
        if (this->queue_processor_method != NULL) {
            this->queue_processor_method->join();
            delete this->queue_processor_method;
            this->queue_processor_method = NULL;
        }
    }

   protected:
    void queue_processor() {
        // GRPC stuff

        // TODO: XXX Review allocation performance in all this method
        set<string> single_answer;                         // Auxiliary set of handles.
        unordered_map<string, unsigned int> joint_answer;  // Auxiliary joint count of handles.
        // HandleTrie *joint_answer = new HandleTrie(HANDLE_HASH_SIZE - 1);

        // Protobuf data structures
        dasproto::HandleList* handle_list;   // will contain single_answer (can't be used directly
                                             // because it's a list, not a set.
        dasproto::HandleCount handle_count;  // Counting of how many times each handle appeared
                                             // in all single_entry
        dasproto::Ack* ack;                  // Command return

        shared_ptr<AtomDB> db = AtomDBSingleton::get_instance();
        shared_ptr<atomdb_api_types::HandleList> query_result;
        stack<string> execution_stack;
        unsigned int weight_sum;
        unsigned int correlated_count = 0;
        unsigned int stimulated_count = 0;

#ifdef DEBUG
        unsigned int count_total_processed = 0;
#endif

        // handle_list.set_context(this->query_context);
        do {
            if (this->is_flow_finished() && this->answers_queue.empty()) {
                break;
            }
            bool idle_flag = true;
            HandlesAnswer* handles_answer;
            string handle;
            unsigned int count;
            while ((handles_answer = (HandlesAnswer*) this->answers_queue.dequeue()) != NULL) {
                if (stimulated_count == MAX_STIMULATE_COUNT) {
                    continue;
                }
#ifdef DEBUG
                count_total_processed++;
                if ((count_total_processed % 1000) == 0) {
                    cout << "RemoteSink::attention_broker_postprocess_method() count_total_processed: "
                         << count_total_processed << endl;
                }
#endif
                for (unsigned int i = 0; i < handles_answer->handles_size; i++) {
                    execution_stack.push(string(handles_answer->handles[i]));
                }
                while (!execution_stack.empty()) {
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
                    // joint_answer->insert(handle, new AccumulatorValue());
                    //  Gets targets and stack them
                    query_result = db->query_for_targets((char*) handle.c_str());
                    if (query_result != NULL) {  // if handle is link
                        unsigned int query_result_size = query_result->size();
                        for (unsigned int i = 0; i < query_result_size; i++) {
                            execution_stack.push(string(query_result->get_handle(i)));
                        }
                    }
                }
                // handle_list.mutable_list()->Clear();
                // handle_list.clear_list();
                auto stub = dasproto::AttentionBroker::NewStub(
                    grpc::CreateChannel(this->attention_broker_address,
                                        grpc::InsecureChannelCredentials()));  // XXXXX Move this up
                handle_list = new dasproto::HandleList();
                handle_list->set_context(this->query_context);
                for (auto handle_it : single_answer) {
                    handle_list->add_list(handle_it);
                }
                single_answer.clear();
                ack = new dasproto::Ack();
#ifdef DEBUG
                // cout << "RemoteSink::attention_broker_postprocess_method() requesting CORRELATE" <<
                // endl;
#endif
                stub->correlate(new grpc::ClientContext(), *handle_list, ack);
                if (ack->msg() != "CORRELATE") {
                    Utils::error("Failed GRPC command: AttentionBroker::correlate()");
                }
                idle_flag = false;
                if (++correlated_count == MAX_CORRELATIONS_WITHOUT_STIMULATE) {
                    correlated_count = 0;
                    for (auto const& pair : joint_answer) {
                        (*handle_count.mutable_map())[pair.first] = pair.second;
                        weight_sum += pair.second;
                    }
                    (*handle_count.mutable_map())["SUM"] = weight_sum;
                    ack = new dasproto::Ack();
                    auto stub = dasproto::AttentionBroker::NewStub(grpc::CreateChannel(
                        this->attention_broker_address, grpc::InsecureChannelCredentials()));
#ifdef DEBUG
                    cout << "RemoteSink::attention_broker_postprocess_method() requesting STIMULATE"
                         << endl;
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
        // joint_answer->traverse(true, &visit_function, &joint_answer_map);
        if (correlated_count > 0) {
            weight_sum = 0;
            for (auto const& pair : joint_answer) {
                (*handle_count.mutable_map())[pair.first] = pair.second;
                weight_sum += pair.second;
            }
            (*handle_count.mutable_map())["SUM"] = weight_sum;
            ack = new dasproto::Ack();
            auto stub = dasproto::AttentionBroker::NewStub(
                grpc::CreateChannel(this->attention_broker_address, grpc::InsecureChannelCredentials()));
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
        // delete joint_answer;
        this->set_flow_finished();
    }

    void set_flow_finished() {
        this->flow_finished_mutex.lock();
        this->flow_finished = true;
        this->flow_finished_mutex.unlock();
    }

    bool is_flow_finished() {
        bool answer;
        this->flow_finished_mutex.lock();
        answer = this->flow_finished;
        this->flow_finished_mutex.unlock();
        return answer;
    }

    SharedQueue answers_queue;
    string attention_broker_address;
    thread* queue_processor_method;
    mutex flow_finished_mutex;
    bool flow_finished;
    string query_context;
};

}  // namespace query_engine
