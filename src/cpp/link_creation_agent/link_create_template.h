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

class CustomField;
class LinkCreateTemplate;
struct Node {
    std::string type;
    std::string value;
};

struct Variable {
    std::string name;
};

using CustomFieldTypes = std::variant<std::string, std::shared_ptr<CustomField>>;
using LinkCreateTemplateTypes = std::variant<Variable, Node, std::shared_ptr<LinkCreateTemplate>>;

class CustomField {
   public:
    CustomField(std::string name, CustomFieldTypes value);
    CustomField(std::vector<std::string> args);
    ~CustomField();
    std::string get_name();
    CustomFieldTypes get_value();
    std::string to_string();


   private:
    std::string name;
    CustomFieldTypes value;
};

enum class TargetType { NODE, TEMPLATE };

/**
 * @class LinkCreateTemplate
 */
class LinkCreateTemplate {
   public:
    LinkCreateTemplate(std::vector<std::string> link_template);
    ~LinkCreateTemplate();
    std::string get_link_type();
    std::vector<LinkCreateTemplateTypes> get_targets();
    std::vector<CustomField> get_custom_fields();
    std::string to_string();


   private:
    std::string link_type;
    std::vector<LinkCreateTemplateTypes> targets;
    std::vector<CustomField> custom_fields;
};

}  // namespace link_creation_agent