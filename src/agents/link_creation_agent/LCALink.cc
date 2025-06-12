#include "LCALink.h"

#include <iostream>

#include "Logger.h"

using namespace link_creation_agent;
using namespace std;
using namespace query_engine;

Link::Link() {}

Link::~Link() {}

string Link::get_type() { return this->type; }

vector<LinkTargetTypes> Link::get_targets() { return this->targets; }

void Link::set_type(string type) { this->type = type; }

void Link::add_target(LinkTargetTypes target) { this->targets.push_back(target); }

vector<string> Link::tokenize(bool include_custom_field_size) {
    vector<string> tokens;
    tokens.push_back("LINK");
    tokens.push_back(this->type);
    tokens.push_back(to_string(this->targets.size()));
    if (include_custom_field_size) {
        tokens.push_back(to_string(this->custom_fields.size()));
    }
    for (LinkTargetTypes target : this->targets) {
        if (holds_alternative<string>(target)) {
            tokens.push_back("HANDLE");
            tokens.push_back(get<string>(target));
        }
        if (holds_alternative<shared_ptr<Link>>(target)) {
            for (string token : get<shared_ptr<Link>>(target)->tokenize(include_custom_field_size)) {
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

void Link::set_custom_fields(vector<CustomField> custom_fields) { this->custom_fields = custom_fields; }

void Link::add_custom_field(CustomField custom_field) { this->custom_fields.push_back(custom_field); }

string Link::to_metta_string() {
    // LOG_DEBUG("MMM 0");
    string metta_string = "(";
    bool is_custom_fields_added = false;
    for (LinkTargetTypes target : this->targets) {
        // LOG_DEBUG("MMM 1");
        if (holds_alternative<string>(target)) {
            // LOG_DEBUG("MMM 2");
            metta_string += get<string>(target) + " ";
        }
        if (holds_alternative<shared_ptr<Link>>(target)) {
            // LOG_DEBUG("MMM 3");
            metta_string += get<shared_ptr<Link>>(target)->to_metta_string() + " ";
        }
        if (holds_alternative<Node>(target)) {
            // LOG_DEBUG("MMM 4");
            Node node = get<Node>(target);
            metta_string += node.value + " ";
        }
        if (!is_custom_fields_added) {
            // LOG_DEBUG("MMM 5");
            for (CustomField custom_field : this->custom_fields) {
                metta_string += custom_field.to_metta_string() + " ";
            }
            is_custom_fields_added = true;
        }
    }
    // LOG_DEBUG("MMM 6");
    // remove the last space
    if (metta_string.back() == ' ') {
        metta_string[metta_string.size() - 1] = ')';
    } else {
        metta_string += ")";
    }
    return metta_string;
}

Link Link::untokenize(const vector<string>& tokens, bool include_custom_field_size) {
    size_t cursor = 0;
    return untokenize_link(tokens, cursor, include_custom_field_size);
}

Link Link::untokenize_link(const vector<string>& tokens, size_t& cursor, bool include_custom_field_size) {
    Link link;
    if (tokens[cursor] != "LINK") {
        throw runtime_error("Invalid token: " + tokens[cursor]);
    }
    cursor++;
    link.type = tokens[cursor];
    cursor++;

    int num_targets = stoi(tokens[cursor]);
    cursor++;
    int num_custom_fields = 0;
    if (include_custom_field_size) {
        num_custom_fields = stoi(tokens[cursor]);
        cursor++;
    }
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
            shared_ptr<Link> sub_link =
                make_shared<Link>(untokenize_link(tokens, cursor, include_custom_field_size));
            link.targets.push_back(sub_link);
        } else {
            throw runtime_error("Invalid token: " + tokens[cursor]);
        }
    }
    if (num_custom_fields > 0 && cursor < tokens.size() && tokens[cursor] == "CUSTOM_FIELD") {
        vector<string> custom_field_args;
        for (long unsigned int i = cursor; i < tokens.size(); i++) {
            custom_field_args.push_back(tokens[i]);
        }
        link.add_custom_field(CustomField(custom_field_args));
    }
    return link;
}
