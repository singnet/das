#pragma once

#include <string>

#include "Wildcard.h"

namespace atoms {

class UntypedVariable : public Wildcard {
   public:
    string name;  ///< The name of the variable.

    // ---------------------------------------------------------------------------------------------
    // Constructors, destructors , basic operators and initializers

    /**
     * @brief Construct a UntypedVariable.
     * @param name The name of the node.
     * @param is_toplevel Whether the variable is a toplevel.
     * @throws std::runtime_error if name is empty.
     */
    UntypedVariable(const string& name, bool is_toplevel = false);

    /**
     * @brief Copy constructor.
     * Deeply copies node type, name
     *
     * @param other UntypedVariable to be copied.
     */
    UntypedVariable(const UntypedVariable& other);

    /**
     * @brief Assignment operator.
     * Deeply copies node type, name
     *
     * @param other UntypedVariable to be copied.
     */
    virtual UntypedVariable& operator=(const UntypedVariable& other);

    /**
     * @brief Comparisson operator.
     *
     * @param other UntypedVariable to be compared to.
     */
    virtual bool operator==(const UntypedVariable& other);

    /**
     * @brief Comparisson operator.
     *
     * @param other UntypedVariable to be compared to.
     */
    virtual bool operator!=(const UntypedVariable& other);

    // ---------------------------------------------------------------------------------------------
    // Public API

    /**
     * @brief Validate the UntypedVariable object.
     *
     * Checks that the type is valid (by calling Atom::validate()) and that the name is not empty.
     *
     * @throws std::runtime_error if the name is empty.
     */
    virtual void validate() const override;

    virtual string to_string() const;

    /**
     * @brief Constructs and returns this Atom's handle.
     *
     * @return A newly built handle for this atom.
     */
    virtual string handle() const;

    /**
     * @brief Constructs and returns a MeTTa expression which represents this Atom.
     *
     * @return a MeTTa expression which represents this Atom.
     */
    virtual string metta_representation(HandleDecoder& decoder) const;

    /**
     * @brief Return true iff the passed handle can be assigned to this variable (i.e. if assigning
     * this variable to the passed handle is not inconsistent with the passed assignment).
     *
     * @return true iff the passed handle can be assigned to this variable (i.e. if assigning
     * this variable to the passed handle is not inconsistent with the passed assignment).
     * @param handle Handle of the Atom being matched against this one.
     * @param assignment Assignment object which will be used to validade the assigment of this
     * variable to the passed handle. THIS OBJECT IS MODIFIED BY SIDE-EFFECT.
     * @param decoder disregarded
     */
    virtual bool match(const string& handle, Assignment& assignment, HandleDecoder& decoder) override;

    virtual void tokenize(vector<string>& output);
    virtual void untokenize(vector<string>& tokens);
};

}  // namespace atoms
