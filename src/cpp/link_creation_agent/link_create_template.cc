#include "link_create_template.h"

#include <stdexcept>

using namespace link_creation_agent;
// using LinkCreateTemplateTypes = std::variant<Node, std::shared_ptr<LinkCreateTemplate>>;


CustomField::CustomField(std::string name, CustomFieldTypes value) {
    this->name = name;
    this->value = value;
}

CustomField::~CustomField() {}

std::string CustomField::get_name() { return this->name; }

CustomFieldTypes CustomField::get_value() { return this->value; }

LinkCreateTemplate::LinkCreateTemplate(std::vector<std::string> link_template) {
    if (link_template[0] != "LINK_CREATE") throw std::invalid_argument("Invalid link template");
    this->link_type = link_template[1];
    std::size_t cursor = 1;
    while (cursor < link_template.size()) {
        if (link_template[cursor] == "NODE") {
            Node node;
            node.type = link_template[cursor + 1];
            node.value = link_template[cursor + 2];
            this->targets.push_back(node);
            cursor += 3;
        } else {
            std::vector<std::string> sub_link_template;
            while (cursor < link_template.size() && link_template[cursor] != "NODE") {
                sub_link_template.push_back(link_template[cursor]);
                cursor++;
            }
            LinkCreateTemplateTypes sub_link = std::make_shared<LinkCreateTemplate>(sub_link_template);
            this->targets.push_back(sub_link);
        }
    }
}

LinkCreateTemplate::~LinkCreateTemplate() {}

std::string LinkCreateTemplate::get_link_type() { return this->link_type; }

std::vector<LinkCreateTemplateTypes> LinkCreateTemplate::get_targets() {
    return this->targets;
}

