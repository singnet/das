/**
 * @file link_create_template.h
 * @brief LinkCreateTemplate class to handle link creation templates
 */

#pragma once
#include <string>
#include <tuple>
#include <vector>
#include <variant>
#include <memory>


namespace link_creation_agent {

class CustomField;
class LinkCreateTemplate;
struct Node {
    std::string type;
    std::string value;
};

using CustomFieldTypes = std::variant<int, float, std::string, std::shared_ptr<CustomField>>;
using LinkCreateTemplateTypes = std::variant<Node, std::shared_ptr<LinkCreateTemplate>>;


class CustomField {
   public:
    CustomField(std::string name, CustomFieldTypes value);
    ~CustomField();
    std::string get_name();
    CustomFieldTypes get_value();

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

   private:
    std::string link_type;
    std::vector<LinkCreateTemplateTypes> targets;
    std::vector<CustomField> custom_fields;
};

}  // namespace link_creation_agent