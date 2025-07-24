#include "TemplateProcessor.h"

#include "LinkCreateTemplate.h"

using namespace std;
using namespace query_engine;
using namespace link_creation_agent;

vector<shared_ptr<Link>> LinkTemplateProcessor::process_query(shared_ptr<QueryAnswer> query_answer,
                                                              optional<vector<string>> extra_params) {
    vector<shared_ptr<Link>> links;
    if (extra_params == nullopt) {
        Utils::error("Invalid link template");
    }
    if (extra_params.value().front() == "LIST") {
        LinkCreateTemplateList link_create_template_list(extra_params.value());
        for (auto link_template : link_create_template_list.get_templates()) {
            LinkCreateTemplate link_create_template(link_template);
            links.push_back(link_create_template.process_query_answer(query_answer));
        }
    } else {
        LinkCreateTemplate link_template(extra_params.value());
        links.push_back(link_template.process_query_answer(query_answer));
    }
    return links;
}