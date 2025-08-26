#pragma once

#include <string>

#include "Atom.h"

namespace atoms {

class Wildcard : public Atom {
   public:
    // ---------------------------------------------------------------------------------------------
    // Constructors, destructors , basic operators and initializers

    /**
     * @brief Construct a Wildcard.
     * @param type The type of the atom.
     * @throws std::runtime_error if type is empty.
     */
    Wildcard(const string& type, bool is_toplevel = false, const Properties& custom_attributes = {});

    Wildcard(const string& type, const Properties& custom_attributes);

    /**
     * @brief Copy constructor.
     *
     * @param other Wildcard to be copied.
     */
    Wildcard(const Wildcard& other);

    /**
     * @brief Assignment operator.
     *
     * @param other Wildcard to be copied.
     */
    virtual Wildcard& operator=(const Wildcard& other);

    /**
     * @brief Comparisson operator.
     *
     * @param other Wildcard to be compared to.
     */
    virtual bool operator==(const Wildcard& other);

    /**
     * @brief Comparisson operator.
     *
     * @param other Wildcard to be compared to.
     */
    virtual bool operator!=(const Wildcard& other);

    // ---------------------------------------------------------------------------------------------
    // Public API - default implementation for Wildcards

    virtual void validate() const override;
    virtual string to_string() const;
    virtual string schema_handle() const;

    // ---------------------------------------------------------------------------------------------
    // Public API - kept virtual (implemented in subclasses)

    virtual string handle() const = 0;
    virtual string metta_representation(HandleDecoder& decoder) const = 0;
    virtual bool match(const string& handle, Assignment& assignment, HandleDecoder& decoder) = 0;
};
}  // namespace atoms
