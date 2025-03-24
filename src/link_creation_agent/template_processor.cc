#include "template_processor.h"

#include <iostream>

using namespace std;
using namespace query_engine;
using namespace link_creation_agent;

void LinkTemplateProcessor::set_template(vector<string> link_template) {
    this->link_create_template = link_template;
}

void LinkTemplateProcessor::process() {
    if (this->link_create_template.front() == "LIST") {
        LinkCreateTemplateList link_create_template_list(this->link_create_template);
        for (auto link_template : link_create_template_list.get_templates()) {
            shared_ptr<Link> link = process_template_request(link_template);
            this->links.push_back(link);
        }
    } else {
        LinkCreateTemplate link_template(this->link_create_template);
        shared_ptr<Link> link = process_template_request(link_template);
        this->links.push_back(link);
    }
}

shared_ptr<Link> LinkTemplateProcessor::process_template_request(LinkCreateTemplate& link_template) {
    LinkCreateTemplate link_create_template(link_template);
    HandlesAnswer* handles_answer = dynamic_cast<HandlesAnswer*>(query_answer);
    shared_ptr<Link> link = make_shared<Link>();
    link->set_type(link_create_template.get_link_type());
    vector<LinkCreateTemplateTypes> targets = link_create_template.get_targets();
    for (LinkCreateTemplateTypes target : targets) {
        if (holds_alternative<Variable>(target)) {
            string token = get<Variable>(target).name;
            link->add_target(handles_answer->assignment.get(token.c_str()));
        }
        if (holds_alternative<std::shared_ptr<LinkCreateTemplate>>(target)) {
            shared_ptr<LinkCreateTemplate> sub_link = get<std::shared_ptr<LinkCreateTemplate>>(target);
            shared_ptr<Link> sub_link_obj = process_template_request(*sub_link.get());
            link->add_target(sub_link_obj);
        }
        if (holds_alternative<Node>(target)) {
            Node node = get<Node>(target);
            link->add_target(node);
        }
    }
    link->set_custom_fields(link_create_template.get_custom_fields());
    return link;
}

vector<vector<string>> LinkTemplateProcessor::get_links() {
    vector<vector<string>> links;
    for (shared_ptr<Link> link : this->links) {
        vector<string> link_tokens = link->tokenize();
        links.push_back(link_tokens);
    }
    return links;
}

vector<shared_ptr<Link>> LinkTemplateProcessor::get_links_objs() { return this->links; }