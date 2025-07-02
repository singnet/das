#include "LinkCreateTemplate.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>

#include "Utils.h"

using namespace link_creation_agent;
using namespace commons;
using namespace std;

static string get_token(vector<string>& link_template, size_t cursor) {
    if (cursor >= link_template.size()) {
        throw invalid_argument("Can not get token: Invalid arguments");
    }
    return link_template[cursor];
}

LinkCreateTemplate::LinkCreateTemplate(const string& link_type) {
    this->link_type = link_type;
    this->targets = {};
    this->custom_fields = {};
}

static vector<string> parse_sub_custom_field(vector<string>& link_template, size_t& cursor) {
    if (get_token(link_template, cursor) != "CUSTOM_FIELD" || link_template.size() < cursor + 3)
        throw invalid_argument("Can not create Custom Field: Invalid arguments");
    vector<string> custom_field_args;
    int custom_field_size = Utils::string_to_int(get_token(link_template, cursor + 2));
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
            custom_field_size += Utils::string_to_int(get_token(link_template, cursor + 2));
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

static vector<string> parse_sub_link_template(vector<string>& link_template, size_t& cursor) {
    if (get_token(link_template, cursor) != "LINK_CREATE" || link_template.size() < cursor + 4)
        throw invalid_argument("Can not create Link Template: Invalid arguments");
    int sub_link_template_size = Utils::string_to_int(get_token(link_template, cursor + 2));
    int sub_link_custom_field_size = Utils::string_to_int(get_token(link_template, cursor + 3));
    int custom_field_value_size = 0;
    vector<string> sub_link_template;
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
            sub_link_template_size += Utils::string_to_int(get_token(link_template, cursor + 2));
            sub_link_custom_field_size = Utils::string_to_int(get_token(link_template, cursor + 3));
            sub_link_template_size--;
        }
        if (sub_link_custom_field_size > 0 && get_token(link_template, cursor) == "CUSTOM_FIELD") {
            current_ptr = 3 + (Utils::string_to_int(get_token(link_template, cursor + 2)) * 2);
            sub_link_custom_field_size--;
        }

        current_ptr--;
        cursor++;
    }
    return sub_link_template;
}

LinkCreateTemplate::LinkCreateTemplate(vector<string>& link_template) {
    int starting_pos = 0;
    if (get_token(link_template, 0) == "LINK_CREATE") {
        starting_pos = 1;
    }

    this->link_type = get_token(link_template, starting_pos);
    size_t cursor = starting_pos;
    size_t link_template_size = Utils::string_to_int(get_token(link_template, starting_pos + 1));
    size_t custom_field_size = Utils::string_to_int(get_token(link_template, starting_pos + 2));
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
            vector<string> sub_link_template = parse_sub_link_template(link_template, cursor);
            LinkCreateTemplateTypes sub_link = make_shared<LinkCreateTemplate>(sub_link_template);
            this->targets.push_back(sub_link);
        } else if (get_token(link_template, cursor) == "CUSTOM_FIELD") {
            vector<string> sub_custom_field = parse_sub_custom_field(link_template, cursor);
            CustomField custom_field = CustomField(sub_custom_field);
            this->custom_fields.push_back(custom_field);
        } else {
            cursor++;
        }
    }
    if (this->targets.size() != link_template_size || this->custom_fields.size() != custom_field_size) {
        throw invalid_argument("Can not create Link Template: Invalid arguments");
    }
}

LinkCreateTemplate::~LinkCreateTemplate() {}

string LinkCreateTemplate::get_link_type() { return this->link_type; }

vector<LinkCreateTemplateTypes> LinkCreateTemplate::get_targets() { return this->targets; }

vector<CustomField> LinkCreateTemplate::get_custom_fields() { return this->custom_fields; }

string LinkCreateTemplate::to_string() {
    string link_template = "LINK_CREATE " + this->link_type + " " +
                           std::to_string(this->targets.size()) + " " +
                           std::to_string(this->custom_fields.size()) + " ";
    for (auto target : this->targets) {
        if (holds_alternative<Node>(target)) {
            Node node = get<Node>(target);
            link_template += "NODE " + node.type + " " + node.value;
        } else if (holds_alternative<Variable>(target)) {
            Variable var = get<Variable>(target);
            link_template += "VARIABLE " + var.name;
        } else if (holds_alternative<shared_ptr<LinkCreateTemplate>>(target)) {
            shared_ptr<LinkCreateTemplate> sub_link = get<shared_ptr<LinkCreateTemplate>>(target);
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

vector<string> LinkCreateTemplate::tokenize() { return Utils::split(this->to_string(), ' '); }

void LinkCreateTemplate::add_target(LinkCreateTemplateTypes target) { this->targets.push_back(target); }

void LinkCreateTemplate::add_custom_field(CustomField custom_field) {
    this->custom_fields.push_back(custom_field);
}

CustomField::CustomField(const string& name) { this->name = name; }

CustomField::CustomField(vector<string>& custom_fields) {
    if (get_token(custom_fields, 0) != "CUSTOM_FIELD")
        throw invalid_argument("Can not create Custom Field: Invalid arguments");

    size_t cursor = 0;
    string custom_field_name = get_token(custom_fields, 1);
    this->name = custom_field_name;
    cursor += 3;
    while (cursor < custom_fields.size()) {
        if (get_token(custom_fields, cursor) == "CUSTOM_FIELD") {
            vector<string> custom_field_args;
            int sub_custom_field_size = Utils::string_to_int(get_token(custom_fields, cursor + 2));
            string sub_custom_field_name = get_token(custom_fields, cursor + 1);
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
                    sub_custom_field_size += Utils::string_to_int(get_token(custom_fields, cursor + 2));
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
                make_tuple(sub_custom_field_name, make_shared<CustomField>(custom_field)));
        } else {
            this->values.push_back(
                make_tuple(get_token(custom_fields, cursor), get_token(custom_fields, cursor + 1)));
            cursor += 2;
        }
    }
}

CustomField::~CustomField() {}

string CustomField::get_name() { return this->name; }

void CustomField::add_field(const string& name, const CustomFieldTypes& value) {
    this->values.push_back(make_tuple(name, value));
}

vector<tuple<string, CustomFieldTypes>> CustomField::get_values() { return this->values; }

string CustomField::to_string() {
    string custom_field = "CUSTOM_FIELD " + this->name + " " + std::to_string(this->values.size()) + " ";
    for (auto value : this->values) {
        CustomFieldTypes field_value = get<1>(value);
        if (holds_alternative<string>(field_value)) {
            custom_field += get<0>(value) + " ";
            custom_field += get<string>(field_value) + " ";
        } else {
            shared_ptr<CustomField> sub_custom_field = get<shared_ptr<CustomField>>(field_value);
            custom_field += sub_custom_field->to_string();
        }
    }
    return custom_field;
}

string CustomField::to_metta_string() {
    string metta_string = "[";
    for (auto value : this->values) {
        CustomFieldTypes field_value = get<1>(value);
        if (holds_alternative<string>(field_value)) {
            metta_string += get<0>(value) + " " + get<string>(field_value) + " ";
        } else {
            shared_ptr<CustomField> sub_custom_field = get<shared_ptr<CustomField>>(field_value);
            metta_string += sub_custom_field->to_metta_string();
            metta_string += " ";
        }
    }
    if (metta_string.back() == ' ') {
        metta_string[metta_string.size() - 1] = ']';
    } else {
        metta_string += "]";
    }
    return metta_string;
}

CustomField CustomField::untokenize(const vector<string>& tokens) {
    size_t cursor = 0;
    return untokenize(tokens, cursor);
}

CustomField CustomField::untokenize(const vector<string>& tokens, size_t& cursor) {
    // TODO implement untokenize
    return CustomField("");
}

vector<string> CustomField::tokenize() { return Utils::split(this->to_string(), ' '); }

LinkCreateTemplateList::LinkCreateTemplateList(vector<string> link_template) {
    if (get_token(link_template, 0) != "LIST")
        throw invalid_argument("Can not create Link Template List: Invalid arguments");

    size_t cursor = 0;
    size_t link_template_size = Utils::string_to_int(get_token(link_template, 1));
    cursor += 2;
    for (size_t i = 0; i < link_template_size; i++) {
        vector<string> sub_link_template = parse_sub_link_template(link_template, cursor);
        this->templates.push_back(LinkCreateTemplate(sub_link_template));
    }
    if (this->templates.size() != link_template_size) {
        throw invalid_argument("Can not create Link Template List: Invalid arguments");
    }
}

LinkCreateTemplateList::~LinkCreateTemplateList() {}

vector<LinkCreateTemplate> LinkCreateTemplateList::get_templates() { return this->templates; }