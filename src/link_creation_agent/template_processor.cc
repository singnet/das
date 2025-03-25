#include "template_processor.h"

#include <iostream>

using namespace std;
using namespace query_engine;
using namespace link_creation_agent;

std::vector<std::vector<std::string>> LinkTemplateProcessor::process(
    QueryAnswer* query_answer, std::optional<std::vector<std::string>> config) {
        std::vector<shared_ptr<Link>> links;
        if(config != std::nullopt){ 
            if (config.value().front() == "LIST") {
                LinkCreateTemplateList link_create_template_list(config.value());
                for (auto link_template : link_create_template_list.get_templates()) {
                    shared_ptr<Link> link = process_template_request(query_answer, link_template);
                    links.push_back(link);
                }
            } else {
                LinkCreateTemplate link_template(config.value());
                shared_ptr<Link> link = process_template_request(query_answer, link_template);
                links.push_back(link);
            }
        } else{
            throw runtime_error("Invalid link template");
        }

        vector<vector<string>> link_tokens;
        for (shared_ptr<Link> link : links) {
            link_tokens.push_back(link->tokenize());
        }
        return link_tokens;
}

shared_ptr<Link> LinkTemplateProcessor::process_template_request(QueryAnswer* query_answer, LinkCreateTemplate& link_template) {
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
            shared_ptr<Link> sub_link_obj = process_template_request(query_answer, *sub_link.get());
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

