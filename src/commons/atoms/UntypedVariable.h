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
     * @throws std::runtime_error if name is empty.
     */
    UntypedVariable(const string& name);

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
};

}  // namespace atoms
