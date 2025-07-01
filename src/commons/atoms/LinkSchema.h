#pragma once

#include <stack>

#include "Wildcard.h"

namespace atoms {

class LinkSchema : public Wildcard {
   private:
    bool _frozen;
    unsigned int _arity;
    vector<string> _schema;
    vector<string> _composite_type;
    string _composite_type_hash;
    string _atom_handle;
    string _metta_representation;
    stack<tuple<string, string, string>> _atom_stack;
    bool check_not_frozen();

   public:
    // ---------------------------------------------------------------------------------------------
    // Constructors, destructors , basic operators and initializers

    /**
     * @brief Construct a LinkSchema.
     * @param type The type of the link.
     * @param LinkSchema arity
     * @param custom_attributes Custom attributes for the schema.
     */
    LinkSchema(const string& type, unsigned int arity, const Properties& custom_attributes = {});

    /**
     * @brief Copy constructor.
     * Deeply copies link type, targets and all custom attributes.
     *
     * @param other LinkSchema to be copied.
     */
    LinkSchema(const LinkSchema& other);

    /**
     * @brief Assignment operator.
     * Deeply copies link type, targets and all custom attributes.
     *
     * @param other LinkSchema to be copied.
     */
    virtual LinkSchema& operator=(const LinkSchema& other);

    /**
     * @brief Comparisson operator.
     *
     * @param other LinkSchema to be compared to.
     */
    virtual bool operator==(const LinkSchema& other);

    /**
     * @brief Comparisson operator.
     *
     * @param other LinkSchema to be compared to.
     */
    virtual bool operator!=(const LinkSchema& other);

    /**
     * @brief Validate the LinkSchema object.
     *
     * @throws std::runtime_error if the number of targets is less than MINIMUM_TARGETS_SIZE.
     */
    void validate() const;

    // ---------------------------------------------------------------------------------------------
    // Public Atom API

    virtual string to_string() const;

    /**
     * @brief Returns this Atom's handle.
     *
     * @return Handle for this atom.
     */
    virtual string handle() const;

    /**
     * @brief Returns this Atom's composite type hash.
     *
     * @return composite type hash for this atom.
     */
    virtual string composite_type_hash(HandleDecoder& decoder) const;

    /**
     * @brief Returns this Atom's composite type hash.
     *
     * @return composite type for this atom.
     */
    virtual vector<string> composite_type(HandleDecoder& decoder) const;

    /**
     * @brief Constructs and returns a MeTTa expression which represents this Atom.
     *
     * @return a MeTTa expression which represents this Atom.
     */
    virtual string metta_representation(HandleDecoder& decoder) const;

    /**
     * @brief Return this LinkSchema's arity
     * @return this LinkSchema's arity.
     */
    unsigned int arity() const;

    // ---------------------------------------------------------------------------------------------
    // Public API to build LinkSchema objects

    void add_target(const string& schema_handle,
                    const string& composite_type_hash,
                    const string& metta_representation);

    void stack_node(const string& type, const string& name);

    void stack_untyped_variable(const string& name);

    void stack_link(const string& type, unsigned int link_arity);

    void build();
};
}  // namespace atoms
