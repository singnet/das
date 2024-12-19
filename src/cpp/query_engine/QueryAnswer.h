#ifndef _QUERY_ENGINE_QUERYANSWER_H
#define _QUERY_ENGINE_QUERYANSWER_H

#include <string>
#include "expression_hasher.h"

// If any of these constants are set to numbers greater than 999, we need
// to fix QueryAnswer.tokenize() properly
#define MAX_VARIABLE_NAME_SIZE ((unsigned int) 100)
#define MAX_NUMBER_OF_VARIABLES_IN_QUERY ((unsigned int) 100)
#define MAX_NUMBER_OF_OPERATION_CLAUSES ((unsigned int) 100)

using namespace std;

namespace query_engine {

/**
 * This class is the representation of a set of variable assignments. It's a set because each
 * variable can be assigned to exactly one value and the order of assignments is irrelevant.
 *
 *     "label1" -> "value1"
 *     "label2" -> "value2"
 *     ...
 *     "labelN" -> "valueN"
 */
class Assignment {

    friend class QueryAnswer;

    public:

        /**
         * Basic constructor.
         */
        Assignment();

        /**
         * Destructor.
         */
        ~Assignment();

        /**
         * Assign a value to a label.
         *
         * If the label have already an assigned value, assign() will check if the value is the
         * same. If not, nothing is done and false is returned. If the value is the same, or
         * if the label haven't been assigned yet, true is returned.
         *
         * @param label Label 
         * @param value Value to be assigned to the passed label.
         * @return true iff the label have no value assigned to it or if the passed value is
         *         the same as the currently assigned value.
         */
        bool assign(const char *label, const char *value);

        /**
         * Returns the value assigned to a given label or NULL if no value is assigned to it.
         *
         * @param label Label to be search for.
         * @return The value assigned to a given label or NULL if no value is assigned to it.
         */
        const char *get(const char *label);

        /**
         * Returns true if the passed Assignment is compatible with this one or false otherwise.
         *
         * For two Assignments to be considered compatible, all the labels they share must be
         * assigned to the same value. Labels defined in only one of the Assignments aren't
         * taken into account (so assignments with no common labels will always be compatible).
         *
         * Empty Assignments are compatible with any other Assignment.
         */
        bool is_compatible(const Assignment &other);

        /**
         * Shallow copy operation. No allocation of labels or values are performed.
         *
         * @param other Assignment to be copied from.
         */
        void copy_from(const Assignment &other);

        /**
         * Adds assignments from other Assignment by making a shallow copy of labels and values.
         *
         * Labels present in both Assignments are disregarded. So, for instance, if 'this' has:
         *
         *     "label1"-> "value1"
         *     "label2"-> "value2"
         *     "label3"-> "value3"
         *
         * and 'other' has:
         *
         *     "label1"-> "valueX"
         *     "label2"-> "value2"
         *     "label4"-> "value4"
         *
         * The result in 'this' after add_assignment() would be:
         *
         *     "label1"-> "value1"
         *     "label2"-> "value2"
         *     "label3"-> "value3"
         *     "label4"-> "value4"
         */
        void add_assignments(const Assignment &other);

        /**
         * Returns the number of labels in this assignment.
         *
         * @return The number of labels in this assignment.
         */
        unsigned int variable_count();

        /**
         * Returns a string representation of this Node (mainly for debugging; not optimized to
         * production environment).
         */
        string to_string();

    private:

        const char *labels[MAX_NUMBER_OF_VARIABLES_IN_QUERY];
        const char *values[MAX_NUMBER_OF_VARIABLES_IN_QUERY];
        unsigned int size;
};

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
    const char *handles[MAX_NUMBER_OF_OPERATION_CLAUSES];

    /**
     * Number of handles in this QueryAnswer.
     */
    unsigned int handles_size;

    /**
     * Estimated importance of this QueryAnswer based on the importance of its constituents.
     */
    double importance;

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
    QueryAnswer(const char *handle, double importance);

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
    void add_handle(const char *handle);

    /**
     * Merges this QueryAnswer with the passed one.
     *
     * @param other QueryAnswer to be merged in this one.
     * @param merge_handles A flag (defaulted to true) to indicate whether the handles should be
     *        merged (in addition to the assignments).
     */
    bool merge(QueryAnswer *other, bool merge_handles = true);

    /**
     *  Make a shallow copy of the passed QueryAnswer.
     *
     *  A new QueryAnswer object is allocated but the assignment and the handles are shallow-copied.
     */
    static QueryAnswer *copy(QueryAnswer *base);

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
     * @return A std::string with tokens separated by spaces which can be used to rebuild this QueryAnswer.
     */
    const string& tokenize();

    /**
     * Rebuild a QueryAnswer baesd in a list of tokens given in a std::string with tokens separated by spaces.
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
    void untokenize(const string &tokens);

    /**
     * Returns a string representation of this Variable (mainly for debugging; not optimized to
     * production environment).
     */
    string to_string();

private:

    string token_representation;
};

} // namespace query_engine

#endif // _QUERY_ENGINE_QUERYANSWER_H
