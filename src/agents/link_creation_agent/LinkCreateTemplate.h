/**
 * @file LinkCreateTemplate.h
 * @brief LinkCreateTemplate class to handle link creation templates
 */

#pragma once
#include <memory>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

#include "Link.h"
#include "LinkSchema.h"
#include "Node.h"
#include "QueryAnswer.h"
#include "UntypedVariable.h"

using namespace std;
using namespace atoms;
using namespace query_engine;
namespace link_creation_agent {

class CustomField;         // Forward declaration
class LinkCreateTemplate;  // Forward declaration

/**
 * @typedef CustomFieldTypes
 * @brief A variant type that can hold either a string or a shared_ptr to a CustomField.
 */
using CustomFieldTypes = variant<string, shared_ptr<CustomField>>;
/**
 * @typedef LinkCreateTemplateTypes
 * @brief A variant type that can hold either a LinkCreateTemplate, Node, UntypedVariable, Link or
 * LinkSchema.
 */
using LinkCreateTemplateTypes = variant<shared_ptr<LinkCreateTemplate>,
                                        shared_ptr<Node>,
                                        shared_ptr<UntypedVariable>,
                                        shared_ptr<Link>,
                                        shared_ptr<LinkSchema>>;

/**
 * @class CustomField
 * @brief Represents a custom field with a name and a list of values.
 */
class CustomField {
   public:
    CustomField(const string& name);
    /**
     * @brief Constructor that initializes the custom field with a list of custom fields.
     */
    CustomField(vector<string>& custom_fields);
    /**
     * @brief Destructor for the CustomField class.
     */
    ~CustomField();
    /**
     * @brief Gets the name of the custom field.
     * @return The name of the custom field.
     */
    string get_name();
    /**
     * @brief Gets the values of the custom field.
     * @return A vector of tuples containing the name and value of the custom field.
     */
    vector<tuple<string, CustomFieldTypes>> get_values();
    /**
     * @brief Converts the custom field to a string representation.
     * @return A string representation of the custom field.
     */
    string to_string();
    /**
     * @brief Tokenizes the custom field.
     * @return A vector of strings representing the tokenized custom field.
     */
    vector<string> tokenize();
    /**
     * @brief Adds a field to the custom field.
     * @param name The name of the field.
     * @param value The value of the field.
     */
    void add_field(const string& name, const CustomFieldTypes& value);

    Properties to_properties();

    CustomField untokenize(const vector<string>& tokens);

    string to_metta_string();

   private:
    string name;
    vector<tuple<string, CustomFieldTypes>> values;
    CustomField untokenize(const vector<string>& tokens, size_t& cursor);
};

/**
 * @class LinkCreateTemplate
 * @brief Represents a link creation template with a link type, targets, and custom fields.
 */
class LinkCreateTemplate {
   public:
    LinkCreateTemplate(const string& link_type);
    /**
     * @brief Constructor that initializes the link creation template with a list of link templates.
     */
    LinkCreateTemplate(vector<string>& link_template);
    /**
     * @brief Destructor for the LinkCreateTemplate class.
     */
    ~LinkCreateTemplate();
    /**
     * @brief Gets the link type of the template.
     * @return The link type of the template.
     */
    string get_link_type();
    /**
     * @brief Gets the targets of the link creation template.
     * @return A vector of LinkCreateTemplateTypes representing the targets.
     */
    vector<LinkCreateTemplateTypes> get_targets();
    /**
     * @brief Gets the custom fields of the link creation template.
     * @return A vector of CustomField objects representing the custom fields.
     */
    vector<CustomField> get_custom_fields();
    /**
     * @brief Converts the link creation template to a string representation.
     * @return A string representation of the link creation template.
     */
    string to_string();

    /**
     * @brief Tokenizes the link creation template.
     * @return A vector of strings representing the tokenized link creation template.
     */
    vector<string> tokenize();
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

    shared_ptr<Link> process_query_answer(shared_ptr<QueryAnswer> query_answer);

   private:
    string link_type;
    vector<LinkCreateTemplateTypes> targets;
    vector<CustomField> custom_fields = {};
};

class LinkCreateTemplateList {
   public:
    LinkCreateTemplateList(vector<string> link_template);
    ~LinkCreateTemplateList();
    vector<LinkCreateTemplate> get_templates();

   private:
    vector<LinkCreateTemplate> templates;
};

}  // namespace link_creation_agent