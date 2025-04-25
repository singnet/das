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

#define DEBUG

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
    std::vector<std::string> tokenize() {
        std::vector<std::string> tokens;
        tokens.push_back("NODE");
        tokens.push_back(type);
        tokens.push_back(value);
        return tokens;
    }
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
    CustomField(const std::string& name);
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
    /**
     * @brief Adds a field to the custom field.
     * @param name The name of the field.
     * @param value The value of the field.
     */
    void add_field(const std::string& name, const CustomFieldTypes& value);

    CustomField untokenize(const std::vector<std::string>& tokens);

    std::string to_metta_string();

   private:
    std::string name;
    std::vector<std::tuple<std::string, CustomFieldTypes>> values;
    CustomField untokenize(const std::vector<std::string>& tokens, int& cursor);
};

/**
 * @class LinkCreateTemplate
 * @brief Represents a link creation template with a link type, targets, and custom fields.
 */
class LinkCreateTemplate {
   public:
    LinkCreateTemplate(const std::string& link_type);
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
    /**
     * @brief Adds a target to the link creation template.
     * @param target The target to add to the link creation template.
     */
    void add_target(LinkCreateTemplateTypes target);
    /**
     * @brief Adds a custom field to the link creation template.
     * @param custom_field The custom field to add to the link creation template.
     */
    void add_custom_field(CustomField custom_field);

   private:
    std::string link_type;
    std::vector<LinkCreateTemplateTypes> targets;
    std::vector<CustomField> custom_fields = {};
};

class LinkCreateTemplateList {
   public:
    LinkCreateTemplateList(std::vector<std::string> link_template);
    ~LinkCreateTemplateList();
    std::vector<LinkCreateTemplate> get_templates();

   private:
    std::vector<LinkCreateTemplate> templates;
};

}  // namespace link_creation_agent