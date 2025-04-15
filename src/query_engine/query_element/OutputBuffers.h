#pragma once

#include <vector>

#include "QueryNode.h"

using namespace std;

namespace query_element {

class OutputBuffers {
   public:
    OutputBuffers(const string& producer_id, const vector<string>& subsequent_ids) {
        for (auto& subsequent_id : subsequent_ids) {
            this->output_buffers.push_back(
                make_shared<query_node::QueryNodeClient>(producer_id, subsequent_id));
        }
    }

    void add_query_answer(QueryAnswer* query_answer) {
        for (auto output_buffer : this->output_buffers) {
            output_buffer->add_query_answer(query_answer);
        }
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
    vector<shared_ptr<query_node::QueryNodeClient>> output_buffers;
};

}  // namespace query_element