#pragma once

#include <memory>
#include <string>

#include "Atom.h"

namespace atoms {

class Link : public Atom {
   public:
    vector<string> targets;  ///< The handles of the target atoms of the link.

    // ---------------------------------------------------------------------------------------------
    // Constructors, destructors , basic operators and initializers

    /**
     * @brief Construct a Link.
     * @param type The type of the link.
     * @param targets The target handles of the link.
     * @param is_toplevel Whether the link is a toplevel.
     * @param custom_attributes Custom attributes for the atom.
     * @throws std::runtime_error if type is empty or targets.size() < MINIMUM_TARGETS_SIZE.
     */
    Link(const string& type,
         const vector<string>& targets,
         bool is_toplevel = false,
         const Properties& custom_attributes = {});

    Link(const string& type, const vector<string>& targets, const Properties& custom_attributes);

    Link(vector<string>& tokens);

    /**
     * @brief Copy constructor.
     * Deeply copies link type, targets and all custom attributes.
     *
     * @param other Link to be copied.
     */
    Link(const Link& other);

    /**
     * @brief Assignment operator.
     * Deeply copies link type, targets and all custom attributes.
     *
     * @param other Link to be copied.
     */
    virtual Link& operator=(const Link& other);

    /**
     * @brief Comparisson operator.
     * Deeply compare all custom attribute values in addition to link type and targets.
     *
     * @param other Link to be compared to.
     */
    virtual bool operator==(const Link& other);

    /**
     * @brief Comparisson operator.
     * Deeply compare all custom attribute values in addition to link type and targets.
     *
     * @param other Link to be compared to.
     */
    virtual bool operator!=(const Link& other);

    // ---------------------------------------------------------------------------------------------
    // Public API

    /**
     * @brief Validate the Link object.
     *
     * Checks that the type is valid (by calling Atom::validate()) and that the number of targets
     * is at least MINIMUM_TARGETS_SIZE.
     *
     * @throws std::runtime_error if the number of targets is less than MINIMUM_TARGETS_SIZE.
     */
    void validate() const;

    virtual string to_string() const;

    /**
     * @brief Constructs and returns this Atom's handle.
     *
     * @return A newly built handle for this atom.
     */
    virtual string handle() const;

    /**
     * @brief Constructs and returns this Atom's composite type hash.
     *
     * @return A newly built composite type hash for this atom.
     */
    virtual string composite_type_hash(HandleDecoder& decoder) const;

    /**
     * @brief Constructs and returns this Atom's composite type hash.
     *
     * @return A newly built composite type for this atom.
     */
    virtual vector<string> composite_type(HandleDecoder& decoder) const;

    /**
     * @brief Constructs and returns a MeTTa expression which represents this Atom.
     *
     * @return a MeTTa expression which represents this Atom.
     */
    virtual string metta_representation(HandleDecoder& decoder) const;

    /**
     * @brief Return this Link's arity
     * @return this Link's arity.
     */
    virtual unsigned int arity() const;

    /**
     * @brief Return true iff the passed handle equals this Link's handle.
     *
     * @return true iff the passed handle equals this Link's handle.
     * @param handle Handle of the Atom being matched against this one.
     * @param assignment disregarded
     * @param decoder disregarded
     */
    virtual bool match(const string& handle, Assignment& assignment, HandleDecoder& decoder) override;

    virtual void tokenize(vector<string>& output);
    virtual void untokenize(vector<string>& tokens);
};

}  // namespace atoms
