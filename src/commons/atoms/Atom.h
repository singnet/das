#pragma once

#include <string>

#include "HandleTrie.h"
#include "Assignment.h"
#include "Properties.h"
#include "Utils.h"
#include "expression_hasher.h"

using namespace commons;

namespace atoms {

class HandleDecoder;  // Forward declaration

class Atom : public HandleTrie::TrieValue {
   protected:
    static string UNDEFINED_TYPE;

   public:
    static string WILDCARD_STRING;
    static string WILDCARD_HANDLE;
    string type;                   ///< The type of the atom.
    Properties custom_attributes;  ///< Custom attributes for the atom.

    // ---------------------------------------------------------------------------------------------
    // Constructors, destructors , basic operators and initializers

    /**
     * @brief Construct an Atom.
     * @param type The type of the atom.
     * @param custom_attributes Custom attributes for the atom.
     * @throws std::runtime_error if type is empty.
     */
    Atom(const string& type, const Properties& custom_attributes = {});

    /**
     * @brief Copy constructor.
     * Deeply copies atom type and all custom attributes.
     *
     * @param other Atom to be copied.
     */
    Atom(const Atom& other);

    virtual void validate() const;

    /**
     * Empty destructor
     */
    virtual ~Atom() override = default;

    /**
     * @brief Assignment operator.
     * Deeply copies atom type and all custom attributes.
     *
     * @param other Atom to be copied.
     */
    virtual Atom& operator=(const Atom& other);

    /**
     * @brief Comparisson operator.
     * Deeply compare all custom attribute values in addition to atom type.
     *
     * @param other Atom to be compared to.
     */
    virtual bool operator==(const Atom& other);

    /**
     * @brief Comparisson operator.
     * Deeply compare all custom attribute values in addition to atom type.
     *
     * @param other Atom to be compared to.
     */
    virtual bool operator!=(const Atom& other);

    // ---------------------------------------------------------------------------------------------
    // Public static methods

    /**
     * @brief Returns true iff the passed atom is a Node (or a Variable, which is a wildcard Node).
     * @return truee iff the passed atom is a Node (or a Variable, which is a wildcard Node).
     */
    static bool is_node(const Atom& atom) { return atom.arity() == 0; }

    /**
     * @brief Returns truee iff the passed atom is a Link (or a LinkSchema, which is a wildcard Link).
     * @return truee iff the passed atom is a Node (or a LinkSchema, which is a wildcard Link).
     */
    static bool is_link(const Atom& atom) { return atom.arity() > 0; }

    // ---------------------------------------------------------------------------------------------
    // Default implementation for virtual public API

    virtual string to_string() const;

    /**
     * @brief Constructs and returns this Atom's named type hash.
     *
     * Return the hash of Atom type.
     * @return A newly built named type hash for this atom.
     */
    virtual string named_type_hash() const;

    /**
     * @brief Constructs and returns this Atom's composite type hash.
     * @param decoder An object (e.g. AtomDB) which can translate handles into actual atoms
     *
     * Return a vector with one hash which is the hash of Atom'; s type.
     * @return A newly built composite type for this atom.
     */
    virtual vector<string> composite_type(HandleDecoder& decoder) const;

    /**
     * @brief Constructs and returns this Atom's composite type hash.
     * @param decoder An object (e.g. AtomDB) which can translate handles into actual atoms
     *
     * Return the hash of Atom type.
     * @return A newly built composite type hash for this atom.
     */
    virtual string composite_type_hash(HandleDecoder& decoder) const;

    /**
     * @brief Constructs and returns the handle this Atom use when inserted into a schema.
     *
     * Return a handle built by calling the pure virtual method handle().
     * @return A newly built handle for this atom.
     */
    virtual string schema_handle() const;

    /**
     * @brief Return this Atom's arity
     *
     * @return this Atom's arity
     */
    virtual unsigned int arity() const;

    // ---------------------------------------------------------------------------------------------
    // Abstract (pure virtual) API

    /**
     * @brief Constructs and returns this Atom's handle.
     *
     * @return A newly built handle for this atom.
     */
    virtual string handle() const = 0;

    /**
     * @brief Constructs and returns a MeTTa expression which represents this Atom.
     *
     * @return a MeTTa expression which represents this Atom.
     */
    virtual string metta_representation(HandleDecoder& decoder) const = 0;

    /**
     * @brief Match atoms against the passed handle and return true iff there's a match.
     *
     * If a variable assignment is required in order to make the match (e.g. for variables and
     * link_schemas), this assigment is returned by side-effect.
     *
     * @return true iff this atom matches with the one whose handle has been passed.
     * @param handle Handle of the Atom being matched against this one.
     * @param assignment Modified by side-effect when a variable assignment is required in order do
     * do the match.
     * @param decoder A Decoder which may be required in order to convert handles -> actual atoms
     * during the match algorithm.
     */
    virtual bool match(const string& handle, Assignment& assignment, HandleDecoder& decoder) = 0;

    // ---------------------------------------------------------------------------------------------
    // HandleTrie::TrieValue virtual API

    virtual void merge(HandleTrie::TrieValue* other) override {}
};
}  // namespace atoms
