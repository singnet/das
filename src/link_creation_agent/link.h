/**
 * @file link.h
 * @brief Link definition
 */
#pragma once
#include <string>
#include <vector>
#include <variant>
#include "QueryAnswer.h"
#include "HandlesAnswer.h"
#include "link_create_template.h"

using namespace std;
using namespace query_engine;

namespace link_creation_agent
{

    class Link; // forward declaration

    using LinkTargetTypes = std::variant<std::string, std::shared_ptr<Link>>;

    class Link
    {
    public:
        Link(QueryAnswer *query_answer, vector<string> link_template);
        Link(QueryAnswer *query_answer, shared_ptr<LinkCreateTemplate> link_create_template);
        Link();
        ~Link();
        /**
         * @brief Get the type of the link
         * @returns Returns the type of the link
         */
        string get_type();
        /**
         * @brief Get the targets of the link
         * @returns Returns the targets of the link
         */
        vector<LinkTargetTypes> get_targets();
        /**
         * @brief Set the type of the link
         */
        void set_type(string type);
        /**
         * @brief Add a target to the link
         * @param target Target to be added
         */
        void add_target(LinkTargetTypes target);
        /**
         * @brief Tokenize the link
         * @returns Returns the tokenized link
         */
        vector<string> tokenize();

    private:
        string type;
        vector<LinkTargetTypes> targets;
        vector<CustomField> custom_fields;


    };
}