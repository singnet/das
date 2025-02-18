/**
 * @file link_create_template.h
 * @brief LinkCreateTemplate class to handle link creation templates
 */

#pragma once
#include <memory>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

namespace link_creation_agent {

class CustomField;         // Forward declaration
class LinkCreateTemplate;  // Forward declaration

/**
 * @struct Node
 * @brief Represents a node with a type and value.
 *
 * @var Node::type
 * Type of the node.
 *
 * @var Node::value
 * Value of the node.
 */
struct Node {
    std::string type;
    std::string value;
};

/**
 * @struct Variable
 * @brief Represents a variable with a name.
 *
 * @var Variable::name
 * Name of the variable.
 */
struct Variable {
    std::string name;
};

/**
 * @typedef CustomFieldTypes
 * @brief A variant type that can hold either a std::string or a std::shared_ptr to a CustomField.
 */
using CustomFieldTypes = std::variant<std::string, std::shared_ptr<CustomField>>;
/**
 * @typedef LinkCreateTemplateTypes
 * @brief A variant type that can hold either a Variable, Node, or a std::shared_ptr to a
 * LinkCreateTemplate.
 */
using LinkCreateTemplateTypes = std::variant<Variable, Node, std::shared_ptr<LinkCreateTemplate>>;

/**
 * @class CustomField
 * @brief Represents a custom field with a name and a list of values.
 */
class CustomField {
   public:
    /**
     * @brief Constructor that initializes the custom field with a list of custom fields.
     */
    CustomField(std::vector<std::string>& custom_fields);
    /**
     * @brief Destructor for the CustomField class.
     */
    ~CustomField();
    /**
     * @brief Gets the name of the custom field.
     * @return The name of the custom field.
     */
    std::string get_name();
    /**
     * @brief Gets the values of the custom field.
     * @return A vector of tuples containing the name and value of the custom field.
     */
    std::vector<std::tuple<std::string, CustomFieldTypes>> get_values();
    /**
     * @brief Converts the custom field to a string representation.
     * @return A string representation of the custom field.
     */
    std::string to_string();
    /**
     * @brief Tokenizes the custom field.
     * @return A vector of strings representing the tokenized custom field.
     */
    std::vector<std::string> tokenize();

   private:
    std::string name;
    std::vector<std::tuple<std::string, CustomFieldTypes>> values;
};

/**
 * @class LinkCreateTemplate
 * @brief Represents a link creation template with a link type, targets, and custom fields.
 */
class LinkCreateTemplate {
   public:
    /**
     * @brief Constructor that initializes the link creation template with a list of link templates.
     */
    LinkCreateTemplate(std::vector<std::string>& link_template);
    /**
     * @brief Destructor for the LinkCreateTemplate class.
     */
    ~LinkCreateTemplate();
    /**
     * @brief Gets the link type of the template.
     * @return The link type of the template.
     */
    std::string get_link_type();
    /**
     * @brief Gets the targets of the link creation template.
     * @return A vector of LinkCreateTemplateTypes representing the targets.
     */
    std::vector<LinkCreateTemplateTypes> get_targets();
    /**
     * @brief Gets the custom fields of the link creation template.
     * @return A vector of CustomField objects representing the custom fields.
     */
    std::vector<CustomField> get_custom_fields();
    /**
     * @brief Converts the link creation template to a string representation.
     * @return A string representation of the link creation template.
     */
    std::string to_string();

    /**
     * @brief Tokenizes the link creation template.
     * @return A vector of strings representing the tokenized link creation template.
     */
    std::vector<std::string> tokenize();

   private:
    std::string link_type;
    std::vector<LinkCreateTemplateTypes> targets;
    std::vector<CustomField> custom_fields;
};

}  // namespace link_creation_agent