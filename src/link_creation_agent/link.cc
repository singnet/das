#include "link.h"

#include <iostream>

using namespace link_creation_agent;
using namespace std;
using namespace query_engine;

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

void Link::set_custom_fields(vector<CustomField> custom_fields) { this->custom_fields = custom_fields; }
