/**
 * @file link.h
 * @brief Link definition
 */
#pragma once
#include <string>
#include <vector>
#include "QueryAnswer.h"

using namespace std;
using namespace query_engine;

namespace link_creation_agent
{

    class Link
    {
    public:
        Link(string type, vector<string> targets);
        Link(QueryAnswer *query_answer, vector<string> link_template);
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
        vector<string> get_targets();
        /**
         * @brief Set the type of the link
         */
        void set_type(string type);
        /**
         * @brief Set the targets of the link
         */
        void set_targets(vector<string> targets);
        /**
         * @brief Add a target to the link
         * @param target Target to be added
         */
        void add_target(string target);
        /**
         * @brief Tokenize the link
         * @returns Returns the tokenized link
         */
        vector<string> tokenize();
        Link untokenize(string link);

    private:
        string type;
        vector<string> targets;
    };
}