#include "link_create_template.h"

#include <iostream>
#include <stdexcept>

using namespace link_creation_agent;
// using LinkCreateTemplateTypes = std::variant<Node, std::shared_ptr<LinkCreateTemplate>>;

CustomField::CustomField(std::string name, CustomFieldTypes value) {
    this->name = name;
    this->value = value;
}

CustomField::CustomField(std::vector<std::string> args) {
    if (args[0] != "CUSTOM_FIELD")
        throw std::invalid_argument("Can not create Custom Field: Invalid arguments");
    int cursor = 0;
    std::cout << "C" << std::endl;
    int custom_field_size = std::stoi(args[2]);
    std::cout << "Custom Field Size: " << custom_field_size << std::endl;
    // while (cursor < args.size()) {
        // if (args[cursor] == "CUSTOM_FIELD") {
    //         std::vector<std::string> custom_field_args;
    //         std::cout << "D" << std::endl;
    //         int sub_custom_field_size = std::stoi(args[cursor + 2]);
    //         while (cursor < args.size()) {
    //             if (sub_custom_field_size == 0) {
    //                 break;
    //             }
    //             custom_field_args.push_back(args[cursor]);
    //             if (args[cursor] == "CUSTOM_FIELD") {
    //                 custom_field_args.push_back(args[cursor + 1]);
    //                 custom_field_args.push_back(args[cursor + 2]);
    //                 std::cout << "E" << std::endl;
    //                 sub_custom_field_size += std::stoi(args[cursor + 2]);
    //                 sub_custom_field_size--;
    //                 cursor += 2;
    //             } else {
    //                 custom_field_args.push_back(args[cursor + 1]);
    //                 cursor++;
    //                 sub_custom_field_size--;
    //             }
    //             cursor++;
    //         }
    //         CustomField custom_field = CustomField(custom_field_args);
    //         this->value = std::make_shared<CustomField>(custom_field);
    //     } else {
    //         this->name = args[cursor];
    //         this->value = args[cursor + 1];
    //         cursor++;
    //     }
    //     cursor++;
    // }
}

CustomField::~CustomField() {}

std::string CustomField::get_name() { return this->name; }

CustomFieldTypes CustomField::get_value() { return this->value; }

LinkCreateTemplate::LinkCreateTemplate(std::vector<std::string> link_template) {
    // if (link_template[0] != "LINK_CREATE") throw std::invalid_argument("Can not create Link Template:
    // Invalid link template");

    int stating_pos = 0;
    if (link_template[0] == "LINK_CREATE") {
        stating_pos = 1;
    }

    this->link_type = link_template[stating_pos];
    std::size_t cursor = 1;
    std::size_t link_template_size = std::stoi(link_template[stating_pos + 1]);
    // bool is_link_template = false;
    // std::vector<std::string> sub_link_template;
    // int sub_link_template_size = 0;
    while (cursor < link_template.size()) {
        if (link_template[cursor] == "NODE") {
            Node node;
            node.type = link_template[cursor + 1];
            node.value = link_template[cursor + 2];
            this->targets.push_back(node);
            cursor += 2;
        } else if (link_template[cursor] == "VARIABLE") {
            Variable var;
            var.name = link_template[cursor + 1];
            this->targets.push_back(var);
            cursor += 1;
        } else if (link_template[cursor] == "LINK_CREATE") {
            int sub_link_template_size = std::stoi(link_template[cursor + 2]);
            std::vector<std::string> sub_link_template;
            while (cursor < link_template.size()) {
                if (sub_link_template_size == 0) {
                    break;
                }
                sub_link_template.push_back(link_template[cursor]);
                if (link_template[cursor] == "NODE") {
                    sub_link_template.push_back(link_template[cursor + 1]);
                    sub_link_template.push_back(link_template[cursor + 2]);
                    cursor += 2;
                    sub_link_template_size--;
                }

                if (link_template[cursor] == "VARIABLE") {
                    sub_link_template.push_back(link_template[cursor + 1]);
                    cursor += 1;
                    sub_link_template_size--;
                }

                if (link_template[cursor] == "CREATE_LINK") {
                    sub_link_template.push_back(link_template[cursor + 1]);  // link type
                    sub_link_template.push_back(link_template[cursor + 2]);  // link size
                    sub_link_template.push_back(link_template[cursor + 3]);  // custom_field size
                    cursor += 3;
                    sub_link_template_size += std::stoi(link_template[cursor + 2]);
                    sub_link_template_size--;
                }
                cursor++;
            }
            LinkCreateTemplateTypes sub_link = std::make_shared<LinkCreateTemplate>(sub_link_template);
            this->targets.push_back(sub_link);
        } else if (link_template[cursor] == "CUSTOM_FIELD") {
            std::vector<std::string> custom_field_args;
            std::cout << "Custom Field Size: " << link_template[cursor + 2] << std::endl;
            // std::string custom_field_name = link_template[cursor + 1];
            std::cout << "A" << std::endl;
            int custom_field_size = std::stoi(link_template[cursor + 2]);
            while (cursor < link_template.size()) {
                if (custom_field_size == 0) {
                    break;
                }
                custom_field_args.push_back(link_template[cursor]);
                if (link_template[cursor] == "CUSTOM_FIELD") {
                    custom_field_args.push_back(link_template[cursor + 1]);
                    custom_field_args.push_back(link_template[cursor + 2]);
                    std::cout << "B" << std::endl;
                    custom_field_size += std::stoi(link_template[cursor + 2]);
                    custom_field_size--;
                    cursor += 2;

                } else {
                    custom_field_args.push_back(link_template[cursor + 1]);
                    cursor++;
                }
                cursor++;
            }
            CustomField custom_field = CustomField(custom_field_args);
            this->custom_fields.push_back(custom_field);
        } else {
            cursor++;
        }
    }
}

LinkCreateTemplate::~LinkCreateTemplate() {}

std::string LinkCreateTemplate::get_link_type() { return this->link_type; }

std::vector<LinkCreateTemplateTypes> LinkCreateTemplate::get_targets() { return this->targets; }

std::string LinkCreateTemplate::to_string() {
    std::string link_template =
        "LINK_CREATE " + this->link_type + " " + std::to_string(this->targets.size()) + " ";
    for (auto target : this->targets) {
        if (std::holds_alternative<Node>(target)) {
            Node node = std::get<Node>(target);
            link_template += "NODE " + node.type + " " + node.value + " ";
        } else if (std::holds_alternative<Variable>(target)) {
            Variable var = std::get<Variable>(target);
            link_template += "VARIABLE " + var.name + " ";
        } else if (std::holds_alternative<std::shared_ptr<LinkCreateTemplate>>(target)) {
            std::shared_ptr<LinkCreateTemplate> sub_link =
                std::get<std::shared_ptr<LinkCreateTemplate>>(target);
            link_template += sub_link->to_string();
        }
    }
    for (auto custom_field : this->custom_fields) {
        link_template += "CUSTOM_FIELD " + custom_field.to_string() + " ";
    }
    return link_template;
}

std::string CustomField::to_string() {
    std::string custom_field = this->name + " ";
    if (std::holds_alternative<std::string>(this->value)) {
        custom_field += std::get<std::string>(this->value);
    } else if (std::holds_alternative<std::shared_ptr<CustomField>>(this->value)) {
        std::shared_ptr<CustomField> sub_custom_field =
            std::get<std::shared_ptr<CustomField>>(this->value);
        custom_field += sub_custom_field->to_string();
    }
    return custom_field;
}