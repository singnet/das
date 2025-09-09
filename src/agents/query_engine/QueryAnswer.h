#pragma once

#include <string>
#include <vector>

#include "Assignment.h"
#include "QueryAnswer.h"
#include "expression_hasher.h"

using namespace std;
using namespace commons;

// If this constant is set to numbers greater than 999, we need
// to fix QueryAnswer.tokenize() properly
#define MAX_NUMBER_OF_OPERATION_CLAUSES ((unsigned int) 100)

namespace query_engine {

class QueryAnswerElement {
   public:
    enum ElementType { HANDLE, VARIABLE };
    ElementType type;
    unsigned int index;
    string name;
    QueryAnswerElement(unsigned int key) : type(HANDLE), index(key) {}
    QueryAnswerElement(const string& key) : type(VARIABLE), name(key) {}
};

/**
 * This is a candidate answer for a query.
 *
 * Objects of this class are moved through the flow of answers in the query tree.
 * They have a vector of handles, an Assignment and an attached importance value which
 * is calculated using the importance of the elements which have been operated to
 * make the answer.
 *
 * The vector of handles represents Links that, together, represent a candidate answer
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
    vector<string> handles;

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
     * Optional map from handle -> MeTTa expression. Optional because it's set only
     * if a specific parameter is set in the query request and, even so, this map is
     * filled only in the last stage of the query tree, i.e. when the QueryAnswer is
     * just to leave a Sink element.
     */
    map<string, string> metta_expression;

    /**
     * Constructor.
     *
     * @param handle First handle in this QueryAnswer.
     * @param importance Estimated importance of this QueryAnswer.
     */
    QueryAnswer(const string& handle, double importance);

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
    void add_handle(const string& handle);

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

    /**
     * Returns the element indicated by the passed QueryAnswerElement key, it can be either one
     * of the handles in the "handles" vector or the value assigned to one of the variables in
     * "assignment". In the former, the key is the index of the handle in the vector and in the i
     * later, the key is the name of the variable.
     *
     * @param element_key A key indicating which elem,ent is to be returned.
     * $return The element indicated by the passed QueryAnswerElement key.
     */
    string get(const QueryAnswerElement& element_key);

   private:
    string token_representation;
};

}  // namespace query_engine

template <>
struct std::hash<Assignment> {
    std::size_t operator()(const Assignment& k) const {
        if (k.table.size() == 0) {
            return 0;
        }

        std::size_t hash_value = 1;
        for (auto pair : k.table) {
            hash_value =
                hash_value ^
                ((std::hash<string>()(pair.first) ^ (std::hash<string>()(pair.second) << 1)) >> 1);
        }

        return hash_value;
    }
};
