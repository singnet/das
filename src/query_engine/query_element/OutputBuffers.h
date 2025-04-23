#pragma once

#include <vector>

#include "QueryNode.h"
#include "Utils.h"

using namespace std;

namespace query_element {

class OutputBuffers {
   public:
    OutputBuffers(const string& producer_id, const vector<string>& consumers) {
        if (consumers.empty()) {
            Utils::error("Invalid empty consumers");
        }
        for (auto& consumer : consumers) {
            this->output_buffers.push_back(make_shared<query_node::QueryNodeClient>(
                producer_id + std::to_string(OutputBuffers::next_instance_count()), consumer));
        }
    }

    void add_query_answer(QueryAnswer* query_answer) {
        auto first_buffer_iter = this->output_buffers.begin();
        // clang-format off
        for (
            auto buffer_iter = first_buffer_iter + 1;
            buffer_iter != this->output_buffers.end();
            buffer_iter++
        ) {
            (*buffer_iter)->add_query_answer(QueryAnswer::copy(query_answer));
        }
        // clang-format on
        (*first_buffer_iter)->add_query_answer(query_answer);
    }

    void query_answers_finished() {
        for (auto output_buffer : this->output_buffers) {
            output_buffer->query_answers_finished();
        }
    }

    void graceful_shutdown() {
        for (auto output_buffer : this->output_buffers) {
            output_buffer->graceful_shutdown();
        }
    }

    bool is_query_answers_finished() {
        for (auto output_buffer : this->output_buffers) {
            if (!output_buffer->is_query_answers_finished()) {
                return false;
            }
        }
        return true;
    }

   protected:
    static unsigned int next_instance_count() {
        static unsigned int instance_count = 0;
        return instance_count++;
    }

    vector<shared_ptr<query_node::QueryNodeClient>> output_buffers;
};

}  // namespace query_element