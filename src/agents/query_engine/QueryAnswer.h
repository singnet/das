#pragma once

#include <string>

#include "Assignment.h"
#include "QueryAnswer.h"
#include "expression_hasher.h"

using namespace std;
using namespace commons;

namespace query_engine {

/**
 * This is a candidate answer for a query.
 *
 * Objects of this class are moved through the flow of answers in the query tree.
 * They have a set of handles, an Assignment and an attached importance value which
 * is calculated using the importance of the elements which have been operated to
 * make the answer.
 *
 * The set of handles represents Links that, together, represent a candidate answer
 * to the query, under the constraints of the attached assignment of variables. For instance,
 * suppose we have a query like:
 *
 * AND
 *     Inheritance
 *         A
 *         $v1
 *     Inheritance
 *         $v1
 *         B
 *
 * One possible candidate answer could be the pair of links:
 *
 *     (Inheritance A S) and (Inheritance S B)
 *
 * with the attached assignment:
 *
 *     $v1 -> S
 */
class QueryAnswer {
   public:
    /**
     * Handles which are the constituents of this QueryAnswer.
     */
    const char* handles[MAX_NUMBER_OF_OPERATION_CLAUSES];

    /**
     * Number of handles in this QueryAnswer.
     */
    unsigned int handles_size;

    /**
     * Estimated importance of this QueryAnswer based on the importance of its constituents.
     */
    double importance;

    /**
     * Strength value which can be understood in different ways depending on the
     * cognitive algorithm using it.
     */
    double strength;

    /**
     * Underlying assignment of variables which led to this QueryAnswer.
     */
    Assignment assignment;

    /**
     * Constructor.
     *
     * @param handle First handle in this QueryAnswer.
     * @param importance Estimated importance of this QueryAnswer.
     */
    QueryAnswer(const char* handle, double importance);

    /**
     * Constructor.
     *
     * @param importance Estimated importance of this QueryAnswer.
     */
    QueryAnswer(double importance);

    /**
     * Empty constructor.
     */
    QueryAnswer();

    /**
     * Destructor.
     */
    ~QueryAnswer();

    /**
     * Adds a handle to this QueryAnswer.
     *
     * @param handles Handle to be added to this QueryAnswer.
     */
    void add_handle(const char* handle);

    /**
     * Merges this QueryAnswer with the passed one.
     *
     * @param other QueryAnswer to be merged in this one.
     * @param merge_handles A flag (defaulted to true) to indicate whether the handles should be
     *        merged (in addition to the assignments).
     */
    bool merge(QueryAnswer* other, bool merge_handles = true);

    /**
     *  Make a shallow copy of the passed QueryAnswer.
     *
     *  A new QueryAnswer object is allocated but the assignment and the handles are shallow-copied.
     */
    static QueryAnswer* copy(QueryAnswer* other);

    /**
     * Tokenizes the QueryAnswer in a single std::string object (tokens separated by spaces).
     *
     * The tokenized string looks like this:
     *
     * N H1 H2 ... HN M L1 V1 L2 V2 ... LM VM
     *
     * N is the number of handles in the QueryAnswer and M is the number of assignments. Hi are the
     * handles and Li Vi are the assignments Li -> Vi
     *
     * @return A std::string with tokens separated by spaces which can be used to rebuild this
     * QueryAnswer.
     */
    const string& tokenize();

    /**
     * Rebuild a QueryAnswer baesd in a list of tokens given in a std::string with tokens separated by
     * spaces.
     *
     * The tokenized string looks like this:
     *
     * N H1 H2 ... HN M L1 V1 L2 V2 ... LM VM
     *
     * N is the number of handles in the QueryAnswer and M is the number of assignments. Hi are the
     * handles and Li Vi are the assignments Li -> Vi
     *
     * @param tokens A std::string with the list of tokens separated by spaces.
     */
    void untokenize(const string& tokens);

    /**
     * Returns a string representation of this QueryAnswer (mainly for debugging; not optimized to
     * production environment).
     */
    string to_string();

   private:
    string token_representation;
};

}  // namespace query_engine

template <>
struct std::hash<Assignment> {
    std::size_t operator()(const Assignment& k) const {
        if (k.size == 0) {
            return 0;
        }

        std::size_t hash_value = 1;
        for (unsigned int i = 0; i < k.size; i++) {
            hash_value = hash_value ^ ((std::hash<string>()(string(k.labels[i])) ^
                                        (std::hash<string>()(string(k.values[i])) << 1)) >>
                                       1);
        }

        return hash_value;
    }
};
