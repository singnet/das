#include "link_create_template.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace link_creation_agent;

// TODO move this to a utils file
static bool is_number(const std::string& s) {
    return !s.empty() &&
           std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}
// TODO move this to a utils file
static int string_to_int(const std::string& s) {
    if (!is_number(s)) {
        throw std::invalid_argument("Can not convert string to int: Invalid arguments");
    }
    return std::stoi(s);
}
// TODO move this to a utils file
static std::string get_token(std::vector<std::string>& link_template, int cursor) {
    if (cursor >= link_template.size()) {
        throw std::invalid_argument("Can not get token: Invalid arguments");
    }
    return link_template[cursor];
}
// TODO move this to a utils file
static std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

LinkCreateTemplate::LinkCreateTemplate(const std::string& link_type) {
    this->link_type = link_type;
    this->targets = {};
    this->custom_fields = {};
}

static std::vector<std::string> parse_sub_custom_field(std::vector<std::string>& link_template,
                                                       size_t& cursor) {
    if (get_token(link_template, cursor) != "CUSTOM_FIELD" || link_template.size() < cursor + 3)
        throw std::invalid_argument("Can not create Custom Field: Invalid arguments");
    std::vector<std::string> custom_field_args;
    int custom_field_size = string_to_int(get_token(link_template, cursor + 2));
    custom_field_args.push_back(get_token(link_template, cursor));      // CUSTOM_FIELD
    custom_field_args.push_back(get_token(link_template, cursor + 1));  // field name
    custom_field_args.push_back(get_token(link_template, cursor + 2));  // field size
    cursor += 3;
    while (cursor < link_template.size()) {
        if (custom_field_size == 0) {
            break;
        }
        custom_field_args.push_back(get_token(link_template, cursor));
        if (get_token(link_template, cursor) == "CUSTOM_FIELD") {
            custom_field_args.push_back(get_token(link_template, cursor + 1));
            custom_field_args.push_back(get_token(link_template, cursor + 2));
            custom_field_size += string_to_int(get_token(link_template, cursor + 2));
            custom_field_size--;
            cursor += 3;
        } else {
            custom_field_args.push_back(get_token(link_template, cursor + 1));
            custom_field_size--;
            cursor += 2;
        }
    }
    return custom_field_args;
}

static std::vector<std::string> parse_sub_link_template(std::vector<std::string>& link_template,
                                                        size_t& cursor) {
    if (get_token(link_template, cursor) != "LINK_CREATE" || link_template.size() < cursor + 4)
        throw std::invalid_argument("Can not create Link Template: Invalid arguments");
    int sub_link_template_size = string_to_int(get_token(link_template, cursor + 2));
    int sub_link_custom_field_size = string_to_int(get_token(link_template, cursor + 3));
    int custom_field_value_size = 0;
    std::vector<std::string> sub_link_template;
    int current_ptr = 0;                                                // link create default size
    sub_link_template.push_back(get_token(link_template, cursor));      // LINK_CREATE
    sub_link_template.push_back(get_token(link_template, cursor + 1));  // link type
    sub_link_template.push_back(get_token(link_template, cursor + 2));  // link size
    sub_link_template.push_back(get_token(link_template, cursor + 3));  // custom field size
    cursor += 4;
    while (cursor < link_template.size()) {
        if (sub_link_template_size == 0 && current_ptr == 0 && sub_link_custom_field_size == 0 &&
            custom_field_value_size == 0) {
            break;
        }
        sub_link_template.push_back(get_token(link_template, cursor));

        if (get_token(link_template, cursor) == "NODE") {
            current_ptr = 3;
            sub_link_template_size--;
        }
        if (get_token(link_template, cursor) == "VARIABLE") {
            current_ptr = 2;
            sub_link_template_size--;
        }
        if (get_token(link_template, cursor) == "LINK_CREATE") {
            current_ptr = 4;
            sub_link_template_size += string_to_int(get_token(link_template, cursor + 2));
            sub_link_custom_field_size = string_to_int(get_token(link_template, cursor + 3));
            sub_link_template_size--;
        }
        if (sub_link_custom_field_size > 0 && get_token(link_template, cursor) == "CUSTOM_FIELD") {
            current_ptr = 3 + (string_to_int(get_token(link_template, cursor + 2)) * 2);
            sub_link_custom_field_size--;
        }

        current_ptr--;
        cursor++;
    }
    return sub_link_template;
}

LinkCreateTemplate::LinkCreateTemplate(std::vector<std::string>& link_template) {
    int starting_pos = 0;
    if (get_token(link_template, 0) == "LINK_CREATE") {
        starting_pos = 1;
    }

    this->link_type = get_token(link_template, starting_pos);
    std::size_t cursor = starting_pos;
    std::size_t link_template_size = string_to_int(get_token(link_template, starting_pos + 1));
    std::size_t custom_field_size = string_to_int(get_token(link_template, starting_pos + 2));
    while (cursor < link_template.size()) {
        if (get_token(link_template, cursor) == "NODE") {
            Node node;
            node.type = get_token(link_template, cursor + 1);
            node.value = get_token(link_template, cursor + 2);
            this->targets.push_back(node);
            cursor += 2;
        } else if (get_token(link_template, cursor) == "VARIABLE") {
            Variable var;
            var.name = get_token(link_template, cursor + 1);
            this->targets.push_back(var);
            cursor += 1;
        } else if (get_token(link_template, cursor) == "LINK_CREATE") {
            std::vector<std::string> sub_link_template = parse_sub_link_template(link_template, cursor);
            LinkCreateTemplateTypes sub_link = std::make_shared<LinkCreateTemplate>(sub_link_template);
            this->targets.push_back(sub_link);
        } else if (get_token(link_template, cursor) == "CUSTOM_FIELD") {
            std::vector<std::string> sub_custom_field = parse_sub_custom_field(link_template, cursor);
            CustomField custom_field = CustomField(sub_custom_field);
            this->custom_fields.push_back(custom_field);
        } else {
            cursor++;
        }
    }
    if (this->targets.size() != link_template_size || this->custom_fields.size() != custom_field_size) {
        throw std::invalid_argument("Can not create Link Template: Invalid arguments");
    }
}

LinkCreateTemplate::~LinkCreateTemplate() {}

std::string LinkCreateTemplate::get_link_type() { return this->link_type; }

std::vector<LinkCreateTemplateTypes> LinkCreateTemplate::get_targets() { return this->targets; }

std::vector<CustomField> LinkCreateTemplate::get_custom_fields() { return this->custom_fields; }

std::string LinkCreateTemplate::to_string() {
    std::string link_template = "LINK_CREATE " + this->link_type + " " +
                                std::to_string(this->targets.size()) + " " +
                                std::to_string(this->custom_fields.size()) + " ";
    for (auto target : this->targets) {
        if (std::holds_alternative<Node>(target)) {
            Node node = std::get<Node>(target);
            link_template += "NODE " + node.type + " " + node.value;
        } else if (std::holds_alternative<Variable>(target)) {
            Variable var = std::get<Variable>(target);
            link_template += "VARIABLE " + var.name;
        } else if (std::holds_alternative<std::shared_ptr<LinkCreateTemplate>>(target)) {
            std::shared_ptr<LinkCreateTemplate> sub_link =
                std::get<std::shared_ptr<LinkCreateTemplate>>(target);
            link_template += sub_link->to_string();
        }
        link_template += " ";
    }
    for (auto custom_field : this->custom_fields) {
        link_template += custom_field.to_string();
    }
    link_template = link_template.substr(0, link_template.size() - 1);
    return link_template;
}

std::vector<std::string> LinkCreateTemplate::tokenize() { return split(this->to_string(), ' '); }

void LinkCreateTemplate::add_target(LinkCreateTemplateTypes target) { this->targets.push_back(target); }

void LinkCreateTemplate::add_custom_field(CustomField custom_field) { this->custom_fields.push_back(custom_field); }

CustomField::CustomField(const std::string& name) { this->name = name; }

CustomField::CustomField(std::vector<std::string>& custom_fields) {
    if (get_token(custom_fields, 0) != "CUSTOM_FIELD")
        throw std::invalid_argument("Can not create Custom Field: Invalid arguments");

    int cursor = 0;
    std::string custom_field_name = get_token(custom_fields, 1);
    this->name = custom_field_name;
    cursor += 3;
    while (cursor < custom_fields.size()) {
        if (get_token(custom_fields, cursor) == "CUSTOM_FIELD") {
            std::vector<std::string> custom_field_args;
            int sub_custom_field_size = string_to_int(get_token(custom_fields, cursor + 2));
            std::string sub_custom_field_name = get_token(custom_fields, cursor + 1);
            custom_field_args.push_back(get_token(custom_fields, cursor));      // CUSTOM_FIELD
            custom_field_args.push_back(get_token(custom_fields, cursor + 1));  // field name
            custom_field_args.push_back(get_token(custom_fields, cursor + 2));  // field size
            cursor += 3;
            while (cursor < custom_fields.size()) {
                if (sub_custom_field_size == 0) {
                    break;
                }

                custom_field_args.push_back(get_token(custom_fields, cursor));
                if (get_token(custom_fields, cursor) == "CUSTOM_FIELD") {
                    sub_custom_field_size += string_to_int(get_token(custom_fields, cursor + 2));
                    custom_field_args.push_back(get_token(custom_fields, cursor + 1));  // field name
                    custom_field_args.push_back(get_token(custom_fields, cursor + 2));  // field size
                    cursor += 3;
                    sub_custom_field_size--;
                } else {
                    custom_field_args.push_back(get_token(custom_fields, cursor + 1));
                    cursor += 2;
                    sub_custom_field_size--;
                }
            }
            CustomField custom_field = CustomField(custom_field_args);
            this->values.push_back(
                std::make_tuple(sub_custom_field_name, std::make_shared<CustomField>(custom_field)));
        } else {
            this->values.push_back(
                std::make_tuple(get_token(custom_fields, cursor), get_token(custom_fields, cursor + 1)));
            cursor += 2;
        }
    }
}

CustomField::~CustomField() {}

std::string CustomField::get_name() { return this->name; }

void CustomField::add_field(const std::string& name, const CustomFieldTypes& value) {
    this->values.push_back(std::make_tuple(name, value));
}

std::vector<std::tuple<std::string, CustomFieldTypes>> CustomField::get_values() { return this->values; }

std::string CustomField::to_string() {
    std::string custom_field =
        "CUSTOM_FIELD " + this->name + " " + std::to_string(this->values.size()) + " ";
    for (auto value : this->values) {
        CustomFieldTypes field_value = std::get<1>(value);
        if (std::holds_alternative<std::string>(field_value)) {
            custom_field += std::get<0>(value) + " ";
            custom_field += std::get<std::string>(field_value) + " ";
        } else {
            std::shared_ptr<CustomField> sub_custom_field =
                std::get<std::shared_ptr<CustomField>>(field_value);
            custom_field += sub_custom_field->to_string();
        }
    }
    return custom_field;
}

std::vector<std::string> CustomField::tokenize() { return split(this->to_string(), ' '); }


LinkCreateTemplateList::LinkCreateTemplateList(std::vector<std::string> link_template) {
    if (get_token(link_template, 0) != "LIST")
        throw std::invalid_argument("Can not create Link Template List: Invalid arguments");

    size_t cursor = 0;
    int link_template_size = string_to_int(get_token(link_template, 1));
    cursor += 2;
    for (int i = 0; i < link_template_size; i++) {
        std::vector<std::string> sub_link_template = parse_sub_link_template(link_template, cursor);
        this->templates.push_back(LinkCreateTemplate(sub_link_template));
    }
    if (this->templates.size() != link_template_size) {
        throw std::invalid_argument("Can not create Link Template List: Invalid arguments");
    }
}


LinkCreateTemplateList::~LinkCreateTemplateList() {}

std::vector<LinkCreateTemplate> LinkCreateTemplateList::get_templates() {
    return this->templates;
}