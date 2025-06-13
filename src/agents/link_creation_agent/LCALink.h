/**
 * @file LCALink.h
 * @brief Link definition
 */
#pragma once
#include <string>
#include <variant>
#include <vector>

#include "LinkCreateTemplate.h"
#include "QueryAnswer.h"

using namespace std;
using namespace query_engine;

namespace link_creation_agent {

class Link;  // forward declaration

using LinkTargetTypes = variant<string, shared_ptr<Link>, Node>;

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
    vector<string> tokenize(bool include_custom_field_size = true);

    string to_metta_string();

    static Link untokenize(const vector<string>& tokens, bool include_custom_field_size = true);

    /**
     * @brief Get the custom fields of the link
     * @returns Returns the custom fields of the link
     */
    vector<CustomField> get_custom_fields();

    void add_custom_field(CustomField custom_field);

    void set_custom_fields(vector<CustomField> custom_fields);

   private:
    string type;
    vector<LinkTargetTypes> targets = {};
    vector<CustomField> custom_fields = {};
    static Link untokenize_link(const vector<string>& tokens,
                                size_t& cursor,
                                bool include_custom_field_size);
};
}  // namespace link_creation_agent
