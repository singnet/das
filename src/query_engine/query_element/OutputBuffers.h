#pragma once

#include <vector>

#include "QueryNode.h"
#include "Utils.h"

using namespace std;

namespace query_element {

/**
 * OutputBuffers is a utility class for managing output buffers for a QueryElement.
 *
 * A QueryElement can produce output that must be sent to one or more consumers.
 * Each consumer is associated with an output buffer, which is managed by OutputBuffers.
 * OutputBuffers provides methods to add a query answer to all output buffers, mark
 * query answers as finished, and check if query answers have finished.
 */
class OutputBuffers {
   public:
    /**
     * Constructs OutputBuffers with given producer ID and list of consumers.
     * Initializes output buffers for each consumer.
     *
     * @param producer_id ID of the producer.
     * @param consumers Vector of consumer IDs.
     */
    OutputBuffers(const string& producer_id, const vector<string>& consumers) {
        if (consumers.empty()) {
            Utils::error("Invalid empty consumers");
        }
        for (auto& consumer_id : consumers) {
            // clang-format off
            this->output_buffers.push_back(
                move(
                    make_unique<query_node::QueryNodeClient>(
                        producer_id + std::to_string(OutputBuffers::next_instance_count()),
                        consumer_id
                    )
                )
            );
            // clang-format on
        }
    }

    /**
     * Destructor.
     * Cleans up all output buffers.
     */
    ~OutputBuffers() { this->output_buffers.clear(); }

    /**
     * Adds a query answer to all output buffers.
     *
     * @param query_answer Pointer to the QueryAnswer to be added.
     */
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

    /**
     * Marks query answers as finished in all output buffers.
     */
    void query_answers_finished() {
        for (auto& output_buffer : this->output_buffers) {
            output_buffer->query_answers_finished();
        }
    }

    /**
     * Initiates a graceful shutdown for all output buffers.
     */
    void graceful_shutdown() {
        for (auto& output_buffer : this->output_buffers) {
            output_buffer->graceful_shutdown();
        }
    }

    /**
     * Checks if query answers have finished in all output buffers.
     *
     * @return True if all output buffers have finished query answers, false otherwise.
     */
    bool is_query_answers_finished() {
        for (auto& output_buffer : this->output_buffers) {
            if (!output_buffer->is_query_answers_finished()) {
                return false;
            }
        }
        return true;
    }

   protected:
    /**
     * Returns the next instance count for naming purposes.
     *
     * @return The next instance count.
     */
    static unsigned int next_instance_count() {
        static unsigned int instance_count = 0;
        return instance_count++;
    }

    vector<unique_ptr<query_node::QueryNodeClient>> output_buffers;
};

}  // namespace query_element