#pragma once

#include <map>
#include <string>
#include <vector>

#include "Assignment.h"
#include "HandleDecoder.h"
#include "Utils.h"
#include "expression_hasher.h"

using namespace std;
using namespace atoms;
using namespace commons;

// If this constant is set to numbers greater than 999, we need
// to fix QueryAnswer.tokenize() properly
#define MAX_NUMBER_OF_OPERATION_CLAUSES ((unsigned int) 100)

namespace query_engine {

class QueryAnswerElement {
   public:
    enum ElementType {
        NOTHING = 0,
        HANDLE,
        PATH,
        VARIABLE,
        ALL_HANDLES,
        ALL_PATH_HANDLES,
        ALL_VARIABLE_VALUES,
        PATH_HOPS,
        EVERYTHING
    };
    ElementType type;
    unsigned int path_index;
    unsigned int element_index;
    string name;
    unsigned int hop_peek_start;
    unsigned int hop_peek_end;
    bool reverse_path;
    bool pop_first;
    bool pop_last;
    QueryAnswerElement() : type(NOTHING), path_index(0), element_index(0), name(""), reverse_path(false) {}
    QueryAnswerElement(ElementType type) : type(type) {
        if ((type <= VARIABLE) || (type == PATH_HOPS)) {
            RAISE_ERROR("Invalid attempt to setup a wildcard selector with type: " +
                        std::to_string(type));
        }
    }
    QueryAnswerElement(unsigned int key) : type(HANDLE), element_index(key) {}
    QueryAnswerElement(unsigned int key_path, unsigned int key_element)
        : type(PATH), path_index(key_path), element_index(key_element) {}
    QueryAnswerElement(const string& key) : type(VARIABLE), name(key) {}
    QueryAnswerElement(unsigned int key_path, unsigned int hop_peek_start, unsigned int hop_peek_end) : type(PATH_HOPS), path_index(key_path), hop_peek_start(hop_peek_start), hop_peek_end(hop_peek_end), reverse_path(false), pop_first(false), pop_last(false) {}
    QueryAnswerElement(unsigned int key_path, unsigned int hop_peek_start, unsigned int hop_peek_end, bool reverse) : type(PATH_HOPS), path_index(key_path), hop_peek_start(hop_peek_start), hop_peek_end(hop_peek_end), reverse_path(reverse), pop_first(false), pop_last(false) {}
    QueryAnswerElement(unsigned int key_path, unsigned int hop_peek_start, unsigned int hop_peek_end, bool reverse, bool pop_first, bool pop_last) : type(PATH_HOPS), path_index(key_path), hop_peek_start(hop_peek_start), hop_peek_end(hop_peek_end), reverse_path(reverse), pop_first(pop_first), pop_last(pop_last) {}
    QueryAnswerElement(const QueryAnswerElement& other)
        : type(other.type),
          path_index(other.path_index),
          element_index(other.element_index),
          name(other.name),
          hop_peek_start(other.hop_peek_start),
          hop_peek_end(other.hop_peek_end),
          reverse_path(other.reverse_path),
          pop_first(other.pop_first),
          pop_last(other.pop_last) {}
    QueryAnswerElement& operator=(const QueryAnswerElement& other) {
        this->type = other.type;
        this->path_index = other.path_index;
        this->element_index = other.element_index;
        this->name = other.name;
        this->hop_peek_start = other.hop_peek_start;
        this->hop_peek_end = other.hop_peek_end;
        this->reverse_path = other.reverse_path;
        this->pop_first = other.pop_first;
        this->pop_last = other.pop_last;
        return *this;
    }
    void set(const string& key) {
        if (this->type == NOTHING) {
            this->type = VARIABLE;
            this->name = key;
        } else {
            RAISE_ERROR("Invalid attempt to reset a QueryAnswerElement");
        }
    }
    void set(unsigned int key) {
        if (this->type == NOTHING) {
            this->type = HANDLE;
            this->element_index = key;
        } else {
            RAISE_ERROR("Invalid attempt to reset a QueryAnswerElement");
        }
    }
    void set(unsigned int key_path, unsigned int key_element) {
        if (this->type == NOTHING) {
            this->type = PATH;
            this->path_index = key_path;
            this->element_index = key_element;
        } else {
            RAISE_ERROR("Invalid attempt to reset a QueryAnswerElement");
        }
    }
    void set(unsigned int key_path, bool reverse, bool pop_first, bool pop_last) {
        if (this->type == NOTHING) {
            this->type = PATH_HOPS;
            this->path_index = key_path;
            this->reverse_path = reverse;
            this->pop_first = pop_first;
            this->pop_last = pop_last;
        } else {
            RAISE_ERROR("Invalid attempt to reset a QueryAnswerElement");
        }
    }
    string to_string() {
        if (this->type == NOTHING) {
            return "-";
        } else if (this->type == HANDLE) {
            return "_" + std::to_string(this->element_index);
        } else if (this->type == VARIABLE) {
            return "$" + this->name;
        } else if (this->type == PATH) {
            return "^" + std::to_string(this->path_index) + "_" + std::to_string(this->element_index);
        } else if (this->type == ALL_HANDLES) {
            return "_*";
        } else if (this->type == ALL_VARIABLE_VALUES) {
            return "$*";
        } else if (this->type == ALL_PATH_HANDLES) {
            return "^*";
        } else if (this->type == PATH_HOPS) {
            string answer;
            if (this->reverse_path) {
                answer += "<";
            } else {
                answer += ">";
            }
            if (this->pop_first) {
                answer += "^";
            }
            if (this->pop_last) {
                answer += "$";
            }
            answer += std::to_string(this->path_index) + "_" + std::to_string(this->hop_peek_start) + "_" + std::to_string(this->hop_peek_end);
            return answer;
        } else if (this->type == EVERYTHING) {
            return "*";
        } else {
            RAISE_ERROR("Invalid QueryAnswerElement type: " + std::to_string(this->type));
            return "";
        }
    }
    static QueryAnswerElement from_string(const string& s) {
        if ((s.size() >= 2) || ((s.size() == 1) && ((s[0] == '-') || (s[0] == '*')))) {
            if (s[0] == '-') {
                return QueryAnswerElement();
            } else if (s[0] == '_') {
                if (s[1] == '*') {
                    return QueryAnswerElement(QueryAnswerElement::ALL_HANDLES);
                } else {
                    return QueryAnswerElement(Utils::string_to_uint(s.substr(1, s.size() - 1)));
                }
            } else if (s[0] == '$') {
                if (s[1] == '*') {
                    return QueryAnswerElement(QueryAnswerElement::ALL_VARIABLE_VALUES);
                } else {
                    return QueryAnswerElement(s.substr(1, s.size() - 1));
                }
            } else if (s[0] == '^') {
                if (s[1] == '*') {
                    return QueryAnswerElement(QueryAnswerElement::ALL_PATH_HANDLES);
                } else {
                    vector<string> keys = Utils::split(s.substr(1, s.size() - 1), '_');
                    if (keys.size() == 2) {
                        return QueryAnswerElement(Utils::string_to_uint(keys[0]),
                                                  Utils::string_to_uint(keys[1]));
                    }
                }
            } else if ((s[0] == '>') || (s[0] == '<')) {
                bool pop_first = false;
                bool pop_last = false;
                unsigned int offset = 1;
                if (s[1] == '^') {
                    pop_first = true;
                    offset++;
                    if (s[2] == '$') {
                        pop_last = true;
                        offset++;
                    }
                } else if (s[1] == '$') {
                    pop_last = true;
                    offset++;
                    if (s[2] == '^') {
                        pop_first = true;
                        offset++;
                    }
                }
                vector<string> args = Utils::split(s.substr(offset, s.size() - offset), '_');
                if (args.size() != 3) {
                    RAISE_ERROR("Invalid PATH_HOPS element string: " + s);
                }
                return QueryAnswerElement(
                    Utils::string_to_uint(args[0]), 
                    Utils::string_to_uint(args[1]), 
                    Utils::string_to_uint(args[2]), 
                    (s[0] == '<'),
                    pop_first,
                    pop_last);
            } else if (s[0] == '*') {
                return QueryAnswerElement(QueryAnswerElement::EVERYTHING);
            }
        }
        RAISE_ERROR("Invalid QueryAnswerElement string representation: " + s);
        return QueryAnswerElement();
    }
    bool is_wildcard() { return (this->type >= ALL_HANDLES); }
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
    enum ImportanceMergeFunction { GREATEST, MULTIPLICATION, SUM };
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
     * Adds a new path in the QueryAnswer (e.g. by CHAIN operator).
     *
     * @return The index opf the newly inserted path, which can be used to make
     * further calls to add_path_element().
     */
    unsigned int add_path();

    /**
     * Adds a new hop in the given path.
     *
     * @param path_index Index of the path which shaw be updated.
     * @param handle The handle of the atom that is the new hop in the path.
     */
    void add_path_element(unsigned int path_index, const string& handle);

    /**
     * Merges this QueryAnswer with the passed one.
     *
     * @param other QueryAnswer to be merged in this one.
     * @param merge_handles A flag (defaulted to true) to indicate whether the handles should be
     *        merged (in addition to the assignments).
     */
    bool merge(QueryAnswer* other,
               bool merge_handles = true,
               ImportanceMergeFunction importance_merger = GREATEST);

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
    string to_string(bool metta_flag = false);

    /**
     * Returns the element indicated by the passed QueryAnswerElement key, it can be either one
     * of the handles in the "handles" vector or the value assigned to one of the variables in
     * "assignment". In the former, the key is the index of the handle in the vector and in the i
     * later, the key is the name of the variable.
     *
     * @param element_key A key indicating which element is to be returned.
     * $return The element indicated by the passed QueryAnswerElement key.
     */
    string get(const QueryAnswerElement& element_key, bool return_empty_when_not_found = false);
    string get(const string& key, bool return_empty_when_not_found = false);
    string get(unsigned int key, bool return_empty_when_not_found = false);
    string get(unsigned int key_path,
               unsigned int key_element,
               bool return_empty_when_not_found = false);

    /**
     * Returns the elements indicated by the passed QueryAnswerElement key, it can be either
     * all handles in the "handles" vector, the values assigned to all the variables in
     * "assignment" or all the handles in the path vectors.
     *
     * @param element_key A key indicating which element is to be returned.
     * @param decoder A decoder capable of mapping handle -> atom (tipically, this is an AtomDB)
     * $return The element indicated by the passed QueryAnswerElement key.
     */
    vector<string> get_all(const QueryAnswerElement& element_key, HandleDecoder *decoder = NULL);

    /**
     * Rewrites the passed query (tokens only, no MeTTa expression allowed) replacing variables
     * by concrete QueryAnswer elements.
     *
     * @param original_query Query to be rewritten.
     * @param A vector with pairs (variable name --> QueryAnswerElement) which specifies
     * which variables in Original_query are supposed to be replaced by the corresponding
     * QueryAnswerElement.
     * @param new_query A new query with the the proper variables replaced by concrete QueryAnswer
     * elements.
     */
    void rewrite_query(const vector<string>& original_query,
                       map<string, QueryAnswerElement>& replacements,
                       vector<string>& new_query);

    unsigned int get_handles_size();
    unsigned int get_paths_size();
    vector<string>& get_handles_vector();
    vector<string>& get_path_vector(unsigned int path_index);
    string compute_hash();

   private:
    void merge_paths(QueryAnswer* other);

    /**
     * Handles which are the constituents of this QueryAnswer.
     */
    vector<vector<string>> handles;

    string token_representation;
};

}  // namespace query_engine
