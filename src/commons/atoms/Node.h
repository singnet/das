#pragma once

#include "Atom.h"

using namespace commons;

namespace atoms {

class Node : public Atom {
   public:
    string name;  ///< The name of the node.

    // ---------------------------------------------------------------------------------------------
    // Constructors, destructors , basic operators and initializers

    /**
     * @brief Construct a Node.
     * @param type The type of the node.
     * @param name The name of the node.
     * @param is_toplevel Whether the node is a toplevel.
     * @param custom_attributes Custom attributes for the atom.
     * @throws std::runtime_error if type or name is empty.
     */
    Node(const string& type,
         const string& name,
         bool is_toplevel = false,
         const Properties& custom_attributes = {});

    Node(const string& type, const string& name, const Properties& custom_attributes);

    /**
     * @brief Copy constructor.
     * Deeply copies node type, name and all custom attributes.
     *
     * @param other Node to be copied.
     */
    Node(const Node& other);

    /**
     * @brief Validate the Node object.
     *
     * Checks that the type is valid (by calling Atom::validate()) and that the name is not empty.
     *
     * @throws std::runtime_error if the name is empty.
     */
    virtual void validate() const override;

    /**
     * @brief Assignment operator.
     * Deeply copies node type, name and all custom attributes.
     *
     * @param other Node to be copied.
     */
    virtual Node& operator=(const Node& other);

    /**
     * @brief Comparisson operator.
     * Deeply compare all custom attribute values in addition to node type and name.
     *
     * @param other Node to be compared to.
     */
    virtual bool operator==(const Node& other);

    /**
     * @brief Comparisson operator.
     * Deeply compare all custom attribute values in addition to node type and name.
     *
     * @param other Node to be compared to.
     */
    virtual bool operator!=(const Node& other);

    // ---------------------------------------------------------------------------------------------
    // Public (inherited from Atom) API

    virtual string to_string() const override;

    /**
     * @brief Constructs and returns this Atom's handle.
     *
     * @return A newly built handle for this atom.
     */
    virtual string handle() const override;

    /**
     * @brief Constructs and returns a MeTTa expression which represents this Atom.
     *
     * @return a MeTTa expression which represents this Atom.
     */
    virtual string metta_representation(HandleDecoder& decoder) const override;

    /**
     * @brief Return true iff the passed handle equals this Node's handle.
     *
     * @return true iff the passed handle equals this Node's handle.
     * @param handle Handle of the Atom being matched against this one.
     * @param assignment disregarded
     * @param decoder disregarded
     */
    virtual bool match(const string& handle, Assignment& assignment, HandleDecoder& decoder) override;

    virtual void tokenize(vector<string>& output);
    virtual void untokenize(vector<string>& tokens);
};
}  // namespace atoms
