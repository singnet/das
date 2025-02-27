/**
 * @file inference_iterator.h
 */
#pragma once

#include "Message.h"
#include <string>
#include <vector>

namespace inference_agent {
template <class NodeType>
class InferenceIterator {
   public:
    InferenceIterator(const string& local_id);
    ~InferenceIterator();

    void setup_buffers();

    void graceful_shutdown();

    bool finished();

    vector<string> pop();

    string get_local_id();

};

}  // namespace inference_agent
