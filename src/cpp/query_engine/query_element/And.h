#ifndef _QUERY_ELEMENT_AND_H
#define _QUERY_ELEMENT_AND_H

#include <queue>
#include <cstring>
#include "Operator.h"

using namespace std;

namespace query_element {


/**
 * QueryElement representing an AND logic operator.
 *
 * And operates on N clauses. Each clause can be either a Source or another Operator.
 */
template <unsigned int N>
class And : public Operator<N> {

public:

    // --------------------------------------------------------------------------------------------
    // Constructors and destructors

    /**
     * Constructor.
     *
     * @param clauses Array with N clauses (each clause is supposed to be a Source or an Operator).
     */
    And(QueryElement **clauses) : Operator<N>(clauses) {
        initialize(clauses);
    }

    /**
     * Constructor.
     *
     * @param clauses Array with N clauses (each clause is supposed to be a Source or an Operator).
     */
    And(const array<QueryElement *, N> &clauses) : Operator<N>(clauses) {
        initialize((QueryElement **) clauses.data());
    }

    /**
     * Destructor.
     */
    ~And() {
        graceful_shutdown();
    }

    // --------------------------------------------------------------------------------------------
    // QueryElement API

    virtual void setup_buffers() {
        Operator<N>::setup_buffers();
        this->operator_thread = new thread(&And::and_operator_method, this);
    }

    virtual void graceful_shutdown() {
        Operator<N>::graceful_shutdown();
        if (this->operator_thread != NULL) {
            this->operator_thread->join();
            this->operator_thread = NULL;
        }
    }
      
    // --------------------------------------------------------------------------------------------
    // Private stuff

private:

    class CandidateRecord {
        public:
        QueryAnswer *answer[N];
        unsigned int index[N];
        double fitness;
        CandidateRecord() {
        }
        CandidateRecord(const CandidateRecord &other) {
            this->fitness = other.fitness;
            memcpy((void *) this->index, (const void *) other.index, N * sizeof(unsigned int));
            memcpy(
                (void *) this->answer,
                (const void *) other.answer,
                N * sizeof(QueryAnswer *));
        }
        CandidateRecord& operator=(const CandidateRecord &other) {
            this->fitness = other.fitness;
            memcpy((void *) this->index, (const void *) other.index, N * sizeof(unsigned int));
            memcpy(
                (void *) this->answer,
                (const void *) other.answer,
                N * sizeof(QueryAnswer *));
            return *this;
        }
        bool operator<(const CandidateRecord &other) const {
            return this->fitness < other.fitness;
        }
        bool operator>(const CandidateRecord &other) const {
            return this->fitness > other.fitness;
        }
        bool operator==(const CandidateRecord &other) const {
            for (unsigned int i = 0; i < N; i++) {
                if (this->index[i] != other.index[i]) {
                    return false;
                }
            }
            return true;
        }
    };

    struct hash_function {
        size_t operator()(const CandidateRecord& record) const {
            size_t hash = record.index[0];
            size_t power = 1;
            for (unsigned int i = 1; i < N; i++) {
                power *= N;
                hash += record.index[i] * power;
            }
            return hash;
        }
     };

    vector<QueryAnswer *> query_answer[N];
    unsigned int next_input_to_process[N];
    priority_queue<CandidateRecord> border;
    unordered_set<CandidateRecord, hash_function> visited;
    bool all_answers_arrived[N];
    bool no_more_answers_to_arrive;
    thread *operator_thread;

    void initialize(QueryElement **clauses) {
        this->operator_thread = NULL;
        for (unsigned int i = 0; i < N; i++) {
            this->next_input_to_process[i] = 0;
            this->all_answers_arrived[i] = false;
        }
        this->no_more_answers_to_arrive = false;
        this->id = "And(";
        for (unsigned int i = 0; i < N; i++) {
            this->id += clauses[i]->id;
            if (i != (N - 1)) {
                this->id += ", ";
            }
        }
        this->id += ")";
    }

    bool ready_to_process_candidate() {
        for (unsigned int i = 0; i < N; i++) {
            if ((! this->all_answers_arrived[i]) &&
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
        QueryAnswer *answer;
        unsigned int all_arrived_count = 0;
        bool no_new_answer = true;
        for (unsigned int i = 0; i < N; i++) {
            while ((answer = this->input_buffer[i]->pop_query_answer()) != NULL) {
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

    void operate_candidate(const CandidateRecord &candidate) {
        QueryAnswer *new_query_answer = QueryAnswer::copy(candidate.answer[0]);
        for (unsigned int i = 1; i < N; i++) {
            if (! new_query_answer->merge(candidate.answer[i])) {
                delete new_query_answer;
                return;
            }
        }
        this->output_buffer->add_query_answer(new_query_answer);
    }

    bool processed_all_input() {
        if (this->border.size() > 0) {
            return false;
        } else {
            for (unsigned int i = 0; i < N; i++) {
                if (this->next_input_to_process[i] < this->query_answer[i].size()) {
                    return false;
                }
            }
        }
        return true;
    }

    void expand_border(const CandidateRecord &last_used_candidate) {
        CandidateRecord candidate;
        unsigned int index_in_queue;
        bool abort_candidate;
        for (unsigned int new_candidate_count = 0; new_candidate_count < N; new_candidate_count++) {
            abort_candidate = false;
            candidate.fitness = 1.0;
            for (unsigned int answer_queue_index = 0; answer_queue_index < N; answer_queue_index++) {
                index_in_queue = last_used_candidate.index[answer_queue_index];
                if (answer_queue_index == new_candidate_count) {
                    index_in_queue++;
                    if (index_in_queue < this->query_answer[answer_queue_index].size()) {
                        if (index_in_queue == this->next_input_to_process[answer_queue_index]) {
                            this->next_input_to_process[answer_queue_index]++;
                        }
                    } else {
                        abort_candidate = true;
                        break;
                    }
                }
                candidate.answer[answer_queue_index] = 
                    this->query_answer[answer_queue_index][index_in_queue];
                candidate.index[answer_queue_index] = index_in_queue;
                candidate.fitness *= candidate.answer[answer_queue_index]->importance;
            }
            if (abort_candidate) {
                continue;
            }
            if (visited.find(candidate) == visited.end()) {
                this->border.push(candidate);
                this->visited.insert(candidate);
            }
        }
    }

    void and_operator_method() {

        do {
            if (QueryElement::is_flow_finished() || 
                this->output_buffer->is_query_answers_finished()) {

                return;
            }

            do {
                if (QueryElement::is_flow_finished()) {
                    return;
                }
                ingest_newly_arrived_answers();
            } while (! ready_to_process_candidate());

            if (processed_all_input()) {
                bool all_finished_flag = true;
                for (unsigned int i = 0; i < N; i++) {
                    if (! this->input_buffer[i]->is_query_answers_finished()) {
                        all_finished_flag = false;
                        break;
                    }
                }
                if (all_finished_flag && 
                    ! this->output_buffer->is_query_answers_finished() &&
                    // processed_all_input() is double-checked on purpose to avoid race condition
                    processed_all_input()) {
                    this->output_buffer->query_answers_finished();
                }
                Utils::sleep();
                continue;
            }

            if (this->border.size() == 0) {
                CandidateRecord candidate;
                double fitness = 1.0;
                for (unsigned int i = 0; i < N; i++) {
                    candidate.answer[i] = this->query_answer[i][this->next_input_to_process[i]],
                    candidate.index[i] = this->next_input_to_process[i];
                    this->next_input_to_process[i]++;
                    fitness *= candidate.answer[i]->importance;
                }
                candidate.fitness = fitness;
                this->border.push(candidate);
                this->visited.insert(candidate);
            }
            CandidateRecord candidate = this->border.top();
            operate_candidate(candidate);
            expand_border(candidate);
            this->border.pop();
        } while (true);
    }
};

} // namespace query_element

#endif // _QUERY_ELEMENT_AND_H
