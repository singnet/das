#pragma once

#include <mutex>

#include "QueryAnswer.h"
#include "QueryElement.h"

using namespace std;

namespace query_element {

/**
 * Superclass for elements which represent logic operators on LinkTemplate results (e.g. AND,
 * OR and NOT).
 *
 * Operator adds the required QueryNode elements to connect either with:
 *
 *     - one or more QueryElement downstream in the query tree (each of them can be either
 *       Operator or SOurce).
 *     - one QueryElement upstream in the query tree which can be another Operator or a Sink.
 */
template <unsigned int N>
class Operator : public QueryElement {
   public:
    // --------------------------------------------------------------------------------------------
    // Constructors and destructors

    /**
     * Constructor.
     *
     * @param clauses Array of QueryElement, each of them a clause in the operation.
     */
    Operator(const array<shared_ptr<QueryElement>, N>& clauses) {
        initialize(clauses);
        this->is_operator = true;
    }

    /**
     * Destructor.
     */
    ~Operator() {
        this->graceful_shutdown();
        for (size_t i = 0; i < N; i++) this->precedent[i] = nullptr;
    }

    // --------------------------------------------------------------------------------------------
    // QueryElement API

    /**
     * Sets up buffers for communication between this operator and its upstream and downstream
     * QueryElements. Initializes a single QueryNodeClient for the upstream connection and
     * N QueryNodeServer elements for the downstream connections, each corresponding to a clause
     * in the operation.
     */
    virtual void setup_buffers() {
        if (this->subsequent_id == "") {
            Utils::error("Invalid empty parent id");
        }
        if (this->id == "") {
            Utils::error("Invalid empty id");
        }

        this->output_buffer = make_shared<QueryNodeClient>(this->id, this->subsequent_id);
        string server_node_id;
        for (unsigned int i = 0; i < N; i++) {
            server_node_id = this->id + "_" + std::to_string(i);
            this->input_buffer[i] = make_shared<QueryNodeServer>(server_node_id);
            this->precedent[i]->subsequent_id = server_node_id;
            this->precedent[i]->setup_buffers();
        }
    }

    /**
     * Gracefully shuts down the QueryNodes attached to the upstream and downstream communication
     * in the query tree.
     */
    virtual void graceful_shutdown() {
        if (is_flow_finished()) {
            return;
        }
        for (unsigned int i = 0; i < N; i++) {
            this->precedent[i]->graceful_shutdown();
        }
        set_flow_finished();
        this->output_buffer->graceful_shutdown();
        for (unsigned int i = 0; i < N; i++) {
            this->input_buffer[i]->graceful_shutdown();
        }
    }

   protected:
    shared_ptr<QueryElement> precedent[N];
    shared_ptr<QueryNodeServer> input_buffer[N];
    shared_ptr<QueryNodeClient> output_buffer;

   private:
    void initialize(const array<shared_ptr<QueryElement>, N>& clauses) {
        if (N > MAX_NUMBER_OF_OPERATION_CLAUSES) {
            Utils::error("Operation exceeds max number of clauses: " + std::to_string(N));
        }
        for (unsigned int i = 0; i < N; i++) {
            precedent[i] = clauses[i];
        }
    }
};

}  // namespace query_element
