#pragma once

#include <cstring>
#include <queue>

#include "Operator.h"
#include "QueryAnswer.h"
#include "StoppableThread.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;

namespace query_element {

/**
 * QueryElement representing an OR logic operator.
 *
 * Or operates on N clauses. Each clause can be either a Source or another Operator.
 */
template <unsigned int N>
class Or : public Operator<N> {
   public:
    // --------------------------------------------------------------------------------------------
    // Constructors and destructors

    /**
     * Constructor.
     *
     * @param clauses Array with N clauses (each clause is supposed to be a Source or an Operator).
     */
    Or(const array<shared_ptr<QueryElement>, N>& clauses) : Operator<N>(clauses) { initialize(clauses); }

    /**
     * Destructor.
     */
    ~Or() { stop(); }

    // --------------------------------------------------------------------------------------------
    // QueryElement API

    virtual void setup_buffers() {
        Operator<N>::setup_buffers();
        this->operator_thread = make_shared<StoppableThread>(this->id);
        this->operator_thread->attach(new thread(&Or::or_operator_method, this, this->operator_thread));
    }

    virtual void stop() {
        if (! stopped()) {
            Operator<N>::stop();
            this->operator_thread->stop();
        }
    }

    // --------------------------------------------------------------------------------------------
    // Private stuff

   private:
    vector<QueryAnswer*> query_answer[N];
    unsigned int next_input_to_process[N];
    bool all_answers_arrived[N];
    bool no_more_answers_to_arrive;
    shared_ptr<StoppableThread>  operator_thread;

    void initialize(const array<shared_ptr<QueryElement>, N>& clauses) {
        for (unsigned int i = 0; i < N; i++) {
            this->next_input_to_process[i] = 0;
            this->all_answers_arrived[i] = false;
        }
        this->no_more_answers_to_arrive = false;
        this->id = "Or(";
        for (unsigned int i = 0; i < N; i++) {
            this->id += clauses[i]->id;
            if (i != (N - 1)) {
                this->id += ", ";
            }
        }
        this->id += ")";
        LOG_INFO(this->id);
    }

    bool ready_to_process_candidate() {
        for (unsigned int i = 0; i < N; i++) {
            if ((!this->all_answers_arrived[i]) &&
                (this->query_answer[i].size() <= (this->next_input_to_process[i] + 1))) {
                return false;
            }
        }
        return true;
    }

    void ingest_newly_arrived_answers() {
        if (this->no_more_answers_to_arrive) {
            return;
        }
        QueryAnswer* answer;
        unsigned int all_arrived_count = 0;
        bool no_new_answer = true;
        for (unsigned int i = 0; i < N; i++) {
            while ((answer = dynamic_cast<QueryAnswer*>(this->input_buffer[i]->pop_query_answer())) !=
                   NULL) {
                no_new_answer = false;
                this->query_answer[i].push_back(answer);
            }
            if (this->input_buffer[i]->is_query_answers_empty() &&
                this->input_buffer[i]->is_query_answers_finished()) {
                this->all_answers_arrived[i] = true;
                all_arrived_count++;
            }
        }
        if (all_arrived_count == N) {
            this->no_more_answers_to_arrive = true;
        } else {
            if (no_new_answer) {
                Utils::sleep();
            }
        }
    }

    bool processed_all_input() {
        for (unsigned int i = 0; i < N; i++) {
            if (this->next_input_to_process[i] < this->query_answer[i].size()) {
                return false;
            }
        }
        return true;
    }

    unsigned int select_answer() {
        unsigned int best_index;
        double best_importance = -1;
        for (unsigned int i = 0; i < N; i++) {
            if (this->next_input_to_process[i] < this->query_answer[i].size()) {
                if (this->query_answer[i][this->next_input_to_process[i]]->importance >
                    best_importance) {
                    best_importance = this->query_answer[i][this->next_input_to_process[i]]->importance;
                    best_index = i;
                }
            }
        }
        if (best_importance < 0) {
            Utils::error("Invalid state in OR operation");
        }
        return best_index;
    }

    void or_operator_method(shared_ptr<StoppableThread> monitor) {
        do {
            if (QueryElement::is_flow_finished() || this->output_buffer->is_query_answers_finished()) {
                return;
            }

            do {
                if (QueryElement::is_flow_finished()) {
                    return;
                }
                ingest_newly_arrived_answers();
            } while (!ready_to_process_candidate());

            if (processed_all_input()) {
                bool all_finished_flag = true;
                for (unsigned int i = 0; i < N; i++) {
                    if (!this->input_buffer[i]->is_query_answers_finished()) {
                        all_finished_flag = false;
                        break;
                    }
                }
                if (all_finished_flag && !this->output_buffer->is_query_answers_finished() &&
                    // processed_all_input() is double-checked on purpose to avoid race condition
                    processed_all_input()) {
                    this->output_buffer->query_answers_finished();
                }
                Utils::sleep();
                continue;
            }

            unsigned int selected_clause = select_answer();
            QueryAnswer* selected_query_answer =
                this->query_answer[selected_clause][this->next_input_to_process[selected_clause]++];
            this->output_buffer->add_query_answer(selected_query_answer);
        } while (true);
    }
};

}  // namespace query_element
