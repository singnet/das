#include "TemplateProcessor.h"

using namespace std;
using namespace query_engine;
using namespace link_creation_agent;

vector<vector<string>> LinkTemplateProcessor::process(shared_ptr<QueryAnswer> query_answer,
                                                      optional<vector<string>> extra_params) {
    vector<shared_ptr<LCALink>> links;
    // if
    if (extra_params == nullopt) {
        throw runtime_error("Invalid link template");
    }
    if (extra_params.value().front() == "LIST") {
        LinkCreateTemplateList link_create_template_list(extra_params.value());
        for (auto link_template : link_create_template_list.get_templates()) {
            shared_ptr<LCALink> link = process_template_request(query_answer, link_template);
            links.push_back(link);
        }
    } else {
        LinkCreateTemplate link_template(extra_params.value());
        shared_ptr<LCALink> link = process_template_request(query_answer, link_template);
        links.push_back(link);
    }
    vector<vector<string>> link_tokens;
    for (shared_ptr<LCALink> link : links) {
        link_tokens.push_back(link->tokenize());
    }
    return link_tokens;
}

shared_ptr<LCALink> LinkTemplateProcessor::process_template_request(shared_ptr<QueryAnswer> query_answer,
                                                                    LinkCreateTemplate& link_template) {
    LinkCreateTemplate link_create_template(link_template);
    // HandlesAnswer* handles_answer = dynamic_cast<HandlesAnswer*>(query_answer);
    shared_ptr<LCALink> link = make_shared<LCALink>();
    link->set_type(link_create_template.get_link_type());
    link->set_custom_fields(link_create_template.get_custom_fields());
    vector<LinkCreateTemplateTypes> targets = link_create_template.get_targets();
    for (LinkCreateTemplateTypes target : targets) {
        if (holds_alternative<Variable>(target)) {
            string token = get<Variable>(target).name;
            link->add_target(query_answer->assignment.get(token.c_str()));
        }
        if (holds_alternative<shared_ptr<LinkCreateTemplate>>(target)) {
            shared_ptr<LinkCreateTemplate> sub_link = get<shared_ptr<LinkCreateTemplate>>(target);
            shared_ptr<LCALink> sub_link_obj = process_template_request(query_answer, *sub_link.get());
            link->add_target(sub_link_obj);
        }
        if (holds_alternative<LCANode>(target)) {
            LCANode node = get<LCANode>(target);
            link->add_target(node);
        }
    }
    return link;
}
