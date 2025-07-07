#pragma once

#include <string>

// If any of these constants are set to numbers greater than 999, we need
// to fix QueryAnswer.tokenize() properly
#define MAX_VARIABLE_NAME_SIZE ((unsigned int) 100)
#define MAX_NUMBER_OF_VARIABLES_IN_QUERY ((unsigned int) 100)
#define MAX_NUMBER_OF_OPERATION_CLAUSES ((unsigned int) 100)

using namespace std;

namespace commons {

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
    bool assign(const char* label, const char* value);

    /**
     * Returns the value assigned to a given label or NULL if no value is assigned to it.
     *
     * @param label Label to be search for.
     * @return The value assigned to a given label or NULL if no value is assigned to it.
     */
    const char* get(const char* label);

    /**
     * Returns true if the passed Assignment is compatible with this one or false otherwise.
     *
     * For two Assignments to be considered compatible, all the labels they share must be
     * assigned to the same value. Labels defined in only one of the Assignments aren't
     * taken into account (so assignments with no common labels will always be compatible).
     *
     * Empty Assignments are compatible with any other Assignment.
     */
    bool is_compatible(const Assignment& other);

    /**
     * Shallow copy operation. No allocation of labels or values are performed.
     *
     * @param other Assignment to be copied from.
     */
    void copy_from(const Assignment& other);

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
    void add_assignments(const Assignment& other);

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

    /**
     * Two Assignments are equal iff they have the exact set of variable lables with
     * the exact same assigned values.
     */
    bool operator==(const Assignment& other) const;

    const char* labels[MAX_NUMBER_OF_VARIABLES_IN_QUERY];
    const char* values[MAX_NUMBER_OF_VARIABLES_IN_QUERY];
    unsigned int size;
};

}  // namespace commons
