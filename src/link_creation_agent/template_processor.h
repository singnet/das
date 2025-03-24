/**
 * @file template_processor.h
 * @brief Template Processor class
 */

#pragma once
#include "link.h"
#include "link_processor.h"

namespace link_creation_agent {
class LinkTemplateProcessor : public LinkProcessor {
   public:
    LinkTemplateProcessor() = default;
    LinkTemplateProcessor(QueryAnswer* query_answer) : LinkProcessor(query_answer) {};
    void set_template(std::vector<std::string> link_template);
    void process() override;
    std::vector<std::vector<std::string>> get_links() override;
    std::vector<shared_ptr<Link>> get_links_objs();
    ~LinkTemplateProcessor() = default;

   private:
    std::vector<shared_ptr<Link>> links;
    std::vector<std::string> link_create_template;
    shared_ptr<Link> process_template_request(LinkCreateTemplate& link_template);
};
}  // namespace link_creation_agent