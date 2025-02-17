/**
 * @file inference_iterator.h
 */
#pragma once

namespace inference_agent {
template <class NodeType>
class InferenceIterator {
   public:
    InferenceIterator(const string& local_id) {
        // this->local_id = local_id;
        // this->setup_buffers();
    }

    ~InferenceIterator() {
        // this->graceful_shutdown();
    }

    void setup_buffers() {
        // this->remote_input_buffer =
        //     make_shared<NodeType>(this->local_id, MessageBrokerType::GRPC);
    }

    void graceful_shutdown() {
        // this->remote_input_buffer->graceful_shutdown();
    }

    bool finished() {
        // return (this->remote_input_buffer->is_answers_finished() &&
        //         this->remote_input_buffer->is_answers_empty());
    }

    string pop() {
        // return (AnswerType*) this->remote_input_buffer->pop_answer();
    }

    string get_local_id() {
        // return this->local_id;
    }
};

}  // namespace inference_agent
