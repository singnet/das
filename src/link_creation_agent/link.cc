#include "link.h"

#include <iostream>

using namespace link_creation_agent;
using namespace std;
using namespace query_engine;

Link::Link(QueryAnswer* query_answer, vector<string> link_template) {
    LinkCreateTemplate link_create_template(link_template);
    HandlesAnswer* handles_answer = dynamic_cast<HandlesAnswer*>(query_answer);

    this->type = link_create_template.get_link_type();
    vector<LinkCreateTemplateTypes> targets = link_create_template.get_targets();
    for (LinkCreateTemplateTypes target : targets) {
        if (holds_alternative<Variable>(target)) {
            string token = get<Variable>(target).name;
            this->targets.push_back(handles_answer->assignment.get(token.c_str()));
        }
        if (holds_alternative<std::shared_ptr<LinkCreateTemplate>>(target)) {
            shared_ptr<LinkCreateTemplate> sub_link = get<std::shared_ptr<LinkCreateTemplate>>(target);
            shared_ptr<Link> sub_link_obj = make_shared<Link>(query_answer, sub_link);
            this->targets.push_back(sub_link_obj);
        }
        if (holds_alternative<Node>(target)) {
            Node node = get<Node>(target);
            this->targets.push_back(node);
        }
    }
    this->custom_fields = link_create_template.get_custom_fields();
}

Link::Link(QueryAnswer* query_answer, shared_ptr<LinkCreateTemplate> link_create_template) {
    HandlesAnswer* handles_answer = dynamic_cast<HandlesAnswer*>(query_answer);
    this->type = link_create_template->get_link_type();
    vector<LinkCreateTemplateTypes> targets = link_create_template->get_targets();
    for (LinkCreateTemplateTypes target : targets) {
        if (holds_alternative<Variable>(target)) {
            string token = get<Variable>(target).name;
            this->targets.push_back(handles_answer->assignment.get(token.c_str()));
        }
        if (holds_alternative<std::shared_ptr<LinkCreateTemplate>>(target)) {
            shared_ptr<LinkCreateTemplate> sub_link = get<std::shared_ptr<LinkCreateTemplate>>(target);
            shared_ptr<Link> sub_link_obj = make_shared<Link>(query_answer, sub_link);
            this->targets.push_back(sub_link_obj);
        }
        if (holds_alternative<Node>(target)) {
            Node node = get<Node>(target);
            this->targets.push_back(node);
        }
    }
    this->custom_fields = link_create_template->get_custom_fields();
}

Link::Link() {}

Link::~Link() {}

string Link::get_type() { return this->type; }

vector<LinkTargetTypes> Link::get_targets() { return this->targets; }

void Link::set_type(string type) { this->type = type; }

void Link::add_target(LinkTargetTypes target) { this->targets.push_back(target); }

vector<string> Link::tokenize() {
    vector<string> tokens;
    tokens.push_back("LINK");
    tokens.push_back(this->type);
    tokens.push_back(to_string(this->targets.size()));
    for (LinkTargetTypes target : this->targets) {
        if (holds_alternative<string>(target)) {
            tokens.push_back("HANDLE");
            tokens.push_back(get<string>(target));
        }
        if (holds_alternative<shared_ptr<Link>>(target)) {
            for (string token : get<shared_ptr<Link>>(target)->tokenize()) {
                tokens.push_back(token);
            }
        }
        if (holds_alternative<Node>(target)) {
            Node node = get<Node>(target);
            tokens.push_back("NODE");
            tokens.push_back(node.type);
            tokens.push_back(node.value);
        }
    }
    for (CustomField custom_field : this->custom_fields) {
        for (string token : custom_field.tokenize()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

vector<CustomField> Link::get_custom_fields() { return this->custom_fields; }

string Link::to_metta_string() {
    string metta_string = "(";
    for (LinkTargetTypes target : this->targets) {
        if (holds_alternative<string>(target)) {
            metta_string += get<string>(target) + " ";
        }
        if (holds_alternative<shared_ptr<Link>>(target)) {
            metta_string += get<shared_ptr<Link>>(target)->to_metta_string() + " ";
        }
        if (holds_alternative<Node>(target)) {
            Node node = get<Node>(target);
            metta_string += node.value + " ";
        }
    }
    // remove the last space
    if (metta_string.back() == ' ') {
        metta_string[metta_string.size() - 1] = ')';
    } else {
        metta_string += ")";
    }
    return metta_string;
}

Link Link::untokenize(const vector<string>& tokens) {
    int cursor = 0;
    return untokenize_link(tokens, cursor);
}

Link Link::untokenize_link(const vector<string>& tokens, int& cursor) {
    Link link;
    if (tokens[cursor] != "LINK") {
        throw std::runtime_error("Invalid token: " + tokens[cursor]);
    }
    cursor++;
    link.type = tokens[cursor];
    cursor++;
    int num_targets = stoi(tokens[cursor]);
    cursor++;
    for (int i = 0; i < num_targets; i++) {
        if (tokens[cursor] == "HANDLE") {
            cursor++;
            string handle = tokens[cursor];
            link.targets.push_back(handle);
            cursor++;
        } else if (tokens[cursor] == "NODE") {
            cursor++;
            string node_type = tokens[cursor];
            cursor++;
            string node_value = tokens[cursor];
            Node node;
            node.type = node_type;
            node.value = node_value;
            link.targets.push_back(node);
            cursor++;
        } else if (tokens[cursor] == "LINK") {
            shared_ptr<Link> sub_link = make_shared<Link>();
            *sub_link = untokenize_link(tokens, cursor);
            link.targets.push_back(sub_link);
        } else {
            throw std::runtime_error("Invalid token: " + tokens[cursor]);
        }
        // TODO: Implement custom field untokenization
    }
    return link;
}
