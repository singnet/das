#pragma once

#include <string>

#include "HandleTrie.h"
#include "Properties.h"
#include "expression_hasher.h"

namespace commons {

class HandleDecoder;  // Forward declaration

class Atom : public HandleTrie::TrieValue {
   public:
    string type;                   ///< The type of the atom.
    Properties custom_attributes;  ///< Custom attributes for the atom.

    /**
     * @brief Construct an Atom.
     * @param type The type of the atom.
     * @param custom_attributes Custom attributes for the atom.
     * @throws std::runtime_error if type is empty.
     */
    Atom(const string& type, const Properties& custom_attributes = {})
        : type(type), custom_attributes(custom_attributes) {
        this->validate();
    }

    /**
     * @brief Copy constructor.
     * Deeply copies atom type and all custom attributes.
     *
     * @param other Atom to be copied.
     */
    Atom(const Atom& other) {
        this->type = other.type;
        this->custom_attributes = other.custom_attributes;
    }

    /**
     * @brief Assignment operator.
     * Deeply copies atom type and all custom attributes.
     *
     * @param other Atom to be copied.
     */
    virtual Atom& operator=(const Atom& other) {
        this->type = other.type;
        this->custom_attributes = other.custom_attributes;
        return *this;
    }

    /**
     * @brief Comparisson operator.
     * Deeply compare all custom attribute values in addition to atom type.
     *
     * @param other Atom to be compared to.
     */
    virtual bool operator==(const Atom& other) {
        return (this->type == other.type) && (this->custom_attributes == other.custom_attributes);
    }

    /**
     * @brief Comparisson operator.
     * Deeply compare all custom attribute values in addition to atom type.
     *
     * @param other Atom to be compared to.
     */
    virtual bool operator!=(const Atom& other) { return !(*this == other); }

    virtual void validate() const {
        if (this->type.empty()) {
            throw runtime_error("Atom type must not be empty");
        }
    }

    virtual ~Atom() override = default;

    virtual string to_string() const {
        return "Atom(type: '" + this->type +
               "', custom_attributes: " + this->custom_attributes.to_string() + ")";
    }

    // HandleTrie::TrieValue virtual API
    virtual void merge(HandleTrie::TrieValue* other) override {}

    /**
     * @brief Constructs and returns this Atom's named type hash.
     *
     * @return A newly built named type hash for this atom.
     */
    virtual string named_type_hash() const {
        char* handle_cstr = ::named_type_hash((char*) type.c_str());
        string handle_string(handle_cstr);
        free(handle_cstr);
        return handle_string;
    }

    /**
     * @brief Constructs and returns this Atom's composite type hash.
     *
     * @return A newly built composite type hash for this atom.
     */
    virtual string composite_type_hash(HandleDecoder& decoder) const { return named_type_hash(); }

    /**
     * @brief Constructs and returns this Atom's composite type hash.
     *
     * @return A newly built composite type for this atom.
     */
    virtual vector<string> composite_type(HandleDecoder& decoder) const {
        return vector<string>({named_type_hash()});
    }

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
};
}  // namespace commons
