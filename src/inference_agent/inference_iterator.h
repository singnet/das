/**
 * @file inference_iterator.h
 */
#pragma once

#include <string>
#include <vector>



namespace inference_agent {
template <class NodeType>
class InferenceIterator {
   public:
    InferenceIterator(const string& local_id){
    this->local_id = local_id;
    this->setup_buffers();
}
    ~InferenceIterator(){
        this->graceful_shutdown();
    }

    void setup_buffers(){
        this->remote_input_buffer = make_shared<NodeType>(this->local_id);
    }

    void graceful_shutdown(){
        this->remote_input_buffer->graceful_shutdown();
    }

    bool finished(){
    return this->remote_input_buffer->is_answers_finished() &&
           this->remote_input_buffer->is_answers_empty();
    }

    vector<string> pop(bool block=true){
    if (!block && this->remote_input_buffer->is_answers_empty()) {
        return {};
    }
    return this->remote_input_buffer->pop_answer();
    }

    string get_local_id(){
        return this->local_id;
    }

    private:
    string local_id;
    shared_ptr<NodeType> remote_input_buffer;   
};

}  // namespace inference_agent
