#pragma once

#include <mutex>

#include "QueryAnswer.h"
#include "QueryElement.h"
#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;

namespace query_element {

/**
 * Superclass for elements which represent logic operators on LinkTemplate results (e.g. AND,
 * OR and NOT).
 *
 * Operator adds the required QueryNode elements to connect either with:
 *
 *     - one or more QueryElement downstream in the query tree (each of them can be either
 *       Operator or Source).
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
        LOG_LOCAL_DEBUG("Creating Operator: " + std::to_string((unsigned long) this));
        initialize(clauses);
        LOG_LOCAL_DEBUG("Operator " + std::to_string((unsigned long) this) + " initialized.");
        this->is_operator = true;
        for (unsigned int i = 0; i < N; i++) {
            this->input_buffer[i] = nullptr;
        }
        this->output_buffer = nullptr;
    }

    /**
     * Destructor.
     */
    virtual ~Operator() {
        LOG_LOCAL_DEBUG("Deleting Operator: " + std::to_string((unsigned long) this) + "...");
        this->graceful_shutdown();
        LOG_LOCAL_DEBUG("Operator " + std::to_string((unsigned long) this) + " is nullifying precedents.");
        for (size_t i = 0; i < N; i++) this->precedent[i] = nullptr;
        LOG_LOCAL_DEBUG("Deleting Operator: " + std::to_string((unsigned long) this) + "... Done");
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
        LOG_LOCAL_DEBUG("Setting up buffers for Operator: " + std::to_string((unsigned long) this));
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
            LOG_LOCAL_DEBUG("Setting up precedent[" + std::to_string(i) + "] buffers for Operator: " + std::to_string((unsigned long) this) + "...");
            this->precedent[i]->setup_buffers();
            LOG_LOCAL_DEBUG("Setting up precedent[" + std::to_string(i) + "] buffers for Operator: " + std::to_string((unsigned long) this) + "... Done");
        }
    }

    /**
     * Gracefully shuts down the QueryNodes attached to the upstream and downstream communication
     * in the query tree.
     */
    virtual void graceful_shutdown() {
        LOG_LOCAL_DEBUG("Gracefully shutting down Operator: " + std::to_string((unsigned long) this) + "...");
        if (is_flow_finished()) {
            LOG_LOCAL_DEBUG("Gracefully shutting down Operator: " + std::to_string((unsigned long) this) + "... Done");
            return;
        }
        for (unsigned int i = 0; i < N; i++) {
            LOG_LOCAL_DEBUG("Gracefully shutting down precedent[" + std::to_string(i) + "] of Operator: " + std::to_string((unsigned long) this) + "...");
            this->precedent[i]->graceful_shutdown();
            LOG_LOCAL_DEBUG("Gracefully shutting down precedent[" + std::to_string(i) + "] of Operator: " + std::to_string((unsigned long) this) + "... Done");
        }
        set_flow_finished();
        Utils::sleep(500);
        if (this->output_buffer != nullptr) {
            LOG_LOCAL_DEBUG("Gracefully shutting down output buffer of Operator: " + std::to_string((unsigned long) this) + "...");
            this->output_buffer->graceful_shutdown();
            LOG_LOCAL_DEBUG("Gracefully shutting down output buffer of Operator: " + std::to_string((unsigned long) this) + "... Done");
        }
        for (unsigned int i = 0; i < N; i++) {
            if (this->input_buffer[i] != nullptr) {
                LOG_LOCAL_DEBUG("Gracefully shutting down input_buffer[" + std::to_string(i) + "] in Operator: " + std::to_string((unsigned long) this) + "...");
                this->input_buffer[i]->graceful_shutdown();
                LOG_LOCAL_DEBUG("Gracefully shutting down input_buffer[" + std::to_string(i) + "] in Operator: " + std::to_string((unsigned long) this) + "... Done");
            }
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
