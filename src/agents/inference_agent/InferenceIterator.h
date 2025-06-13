/**
 * @file InferenceIterator.h
 * @brief InferenceIterator class is a helper
 * class to iterate over the answers of a node
 */
#pragma once

#include <string>
#include <vector>

namespace inference_agent {
/**
 * @brief InferenceIterator class is a helper class to iterate over the answers of a node
 * @tparam NodeType Type of the node
 */
template <class NodeType>
class InferenceIterator {
   public:
    /**
     * @brief Construct a new Inference Iterator object
     * @param local_id Local id of the node
     */
    InferenceIterator(const string& local_id) {
        this->local_id = local_id;
        this->setup_buffers();
    }
    /**
     * @brief Destroy the Inference Iterator object
     */
    ~InferenceIterator() { this->graceful_shutdown(); }

    /**
     * @brief Setup the buffers of the iterator
     */
    void setup_buffers() { this->remote_input_buffer = make_shared<NodeType>(this->local_id); }

    /**
     * @brief Gracefully shutdown the iterator
     */
    void graceful_shutdown() { this->remote_input_buffer->graceful_shutdown(); }

    /**
     * @brief Check if the iterator is finished
     * @return true If the iterator is finished
     * @return false If the iterator is not finished
     */
    bool finished() {
        return this->remote_input_buffer->is_answers_finished() &&
               this->remote_input_buffer->is_answers_empty();
    }

    /**
     * @brief Pop an answer from the iterator
     * @param block Whether to block the pop operation
     */
    vector<string> pop(bool block = true) {
        if (!block && this->remote_input_buffer->is_answers_empty()) {
            return {};
        }
        return this->remote_input_buffer->pop_answer();
    }

    /**
     * @brief Get the local id of the iterator
     * @return string Local id of the iterator
     */
    string get_local_id() { return this->local_id; }

   private:
    string local_id;
    shared_ptr<NodeType> remote_input_buffer;
};

}  // namespace inference_agent
