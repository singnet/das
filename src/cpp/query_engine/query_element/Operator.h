#ifndef _QUERY_ELEMENT_OPERATOR_H
#define _QUERY_ELEMENT_OPERATOR_H

#include <mutex>
#include "QueryElement.h"
#include "QueryAnswer.h"

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
    Operator(const array<QueryElement *, N> &clauses) {
        initialize((QueryElement **) clauses.data());
    }

    /**
     * Constructor.
     *
     * @param clauses Array of QueryElement, each of them a clause in the operation.
     */
    Operator(QueryElement **clauses) {
        initialize(clauses);
    }

    /**
     * Destructor.
     */
    ~Operator() {
        this->graceful_shutdown();
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

        this->output_buffer = shared_ptr<QueryNodeClient>(new QueryNodeClient(this->id, this->subsequent_id));
        string server_node_id;
        for (unsigned int i = 0; i < N; i++) {
            server_node_id = this->id + "_" + to_string(i);
            this->input_buffer[i] = shared_ptr<QueryNodeServer>(new QueryNodeServer(server_node_id));
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

    QueryElement *precedent[N];
    shared_ptr<QueryNodeServer> input_buffer[N];
    shared_ptr<QueryNodeClient> output_buffer;

private:

    void initialize(QueryElement **clauses) {
        if (N > MAX_NUMBER_OF_OPERATION_CLAUSES) {
            Utils::error("Operation exceeds max number of clauses: " + to_string(N));
        }
        for (unsigned int i = 0; i < N; i++) {
            precedent[i] = clauses[i];
        }
    }
};

} // namespace query_element

#endif // _QUERY_ELEMENT_OPERATOR_H
