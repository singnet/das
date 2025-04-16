/**
 * @file link.h
 * @brief Link definition
 */
#pragma once
#include <string>
#include <variant>
#include <vector>

#include "HandlesAnswer.h"
#include "QueryAnswer.h"
#include "link_create_template.h"

using namespace std;
using namespace query_engine;

namespace link_creation_agent {

class Link;  // forward declaration

using LinkTargetTypes = std::variant<std::string, std::shared_ptr<Link>, Node>;

class Link {
   public:
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

    string to_metta_string();

    Link untokenize(const vector<string>& tokens);

    /**
     * @brief Get the custom fields of the link
     * @returns Returns the custom fields of the link
     */
    vector<CustomField> get_custom_fields();

    void add_custom_field(CustomField custom_field);


    void set_custom_fields(vector<CustomField> custom_fields);

   private:
    string type;
    vector<LinkTargetTypes> targets;
    vector<CustomField> custom_fields;
    Link untokenize_link(const vector<string>& tokens, int& cursor);
};
}  // namespace link_creation_agent