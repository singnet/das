#include <algorithm>
#include <array>
#include <vector>

// clang-format off

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

#include "And.h"
#include "LinkTemplate.h"
#include "MettaMapping.h"
#include "MettaParserActions.h"
#include "Or.h"
#include "Terminal.h"
#include "UniqueAssignmentFilter.h"

// clang-format on

using namespace query_engine;

// -------------------------------------------------------------------------------------------------
// Public methods

MettaParserActions::MettaParserActions(shared_ptr<PatternMatchingQueryProxy> proxy) {
    this->proxy = proxy;
    this->current_expression_size = 0;
    this->current_expression_type = LINK;
    this->count_open_expression = 0;
}

MettaParserActions::~MettaParserActions() {}

void MettaParserActions::symbol(const string& name) {
    ParserActions::symbol(name);
    if ((name == MettaMapping::AND_QUERY_OPERATOR) || (name == MettaMapping::OR_QUERY_OPERATOR)) {
        if (name == MettaMapping::AND_QUERY_OPERATOR) {
            this->current_expression_type = AND;
        } else {
            this->current_expression_type = OR;
        }
    } else {
        if ((this->current_expression_type == AND) || (this->current_expression_type == OR)) {
            Utils::error("Invalid query expression: AND/OR can't operate symbols.");
            return;
        }
        this->current_expression_size++;
        this->element_stack.push(make_shared<Terminal>(MettaMapping::SYMBOL_NODE_TYPE, name));
    }
}

void MettaParserActions::variable(const string& value) {
    ParserActions::variable(value);
    if ((this->current_expression_type == AND) || (this->current_expression_type == OR)) {
        Utils::error("Invalid query expression: AND/OR can't operate variables.");
        return;
    }
    this->current_expression_size++;
    this->current_expression_type = LINK_TEMPLATE;
    this->element_stack.push(make_shared<Terminal>(value));
}

void MettaParserActions::literal(const string& value) {
    ParserActions::literal(value);
    if ((this->current_expression_type == AND) || (this->current_expression_type == OR)) {
        Utils::error("Invalid query expression: AND/OR can't operate literals.");
        return;
    }
    this->current_expression_size++;
    this->element_stack.push(make_shared<Terminal>(MettaMapping::SYMBOL_NODE_TYPE, value));
}

void MettaParserActions::literal(int value) {
    ParserActions::literal(value);
    if ((this->current_expression_type == AND) || (this->current_expression_type == OR)) {
        Utils::error("Invalid query expression: AND/OR can't operate literals.");
        return;
    }
    this->current_expression_size++;
    this->element_stack.push(
        make_shared<Terminal>(MettaMapping::SYMBOL_NODE_TYPE, std::to_string(value)));
}

void MettaParserActions::literal(float value) {
    ParserActions::literal(value);
    if ((this->current_expression_type == AND) || (this->current_expression_type == OR)) {
        Utils::error("Invalid query expression: AND/OR can't operate literals.");
        return;
    }
    this->current_expression_size++;
    this->element_stack.push(
        make_shared<Terminal>(MettaMapping::SYMBOL_NODE_TYPE, std::to_string(value)));
}

void MettaParserActions::expression_begin() {
    ParserActions::expression_begin();
    this->expression_size_stack.push(this->current_expression_size);
    this->expression_type_stack.push(this->current_expression_type);
    this->current_expression_size = 0;
    this->current_expression_type = LINK;
    this->count_open_expression++;
}

void MettaParserActions::expression_end(bool toplevel, const string& metta_expression) {
    ParserActions::expression_end(toplevel, metta_expression);
    unsigned int arity = this->current_expression_size;
    if (element_stack.size() < arity) {
        Utils::error("Invalid query expression: too few arguments for expression. Expected: " +
                     std::to_string(arity) + " Stack size: " + std::to_string(element_stack.size()));
        return;
    }
    LOG_DEBUG("Arity: " + std::to_string(arity));
    if ((this->current_expression_type == LINK) || (this->current_expression_type == LINK_TEMPLATE)) {
        vector<shared_ptr<QueryElement>> targets;
        for (unsigned int i = 0; i < arity; i++) {
            targets.push_back(element_stack.top());
            element_stack.pop();
        }
        reverse(targets.begin(), targets.end());
        if (this->current_expression_type == LINK) {
            if (toplevel) {
                Utils::error("Invalid query expression: a link is not a valid pattern matching query");
                return;
            }
            LOG_DEBUG("Pushing LINK");
            this->element_stack.push(make_shared<Terminal>(MettaMapping::EXPRESSION_LINK_TYPE, targets));
        } else {
            LOG_DEBUG("Pushing LINK_TEMPLATE");
            auto new_link_template = make_shared<LinkTemplate>(
                MettaMapping::EXPRESSION_LINK_TYPE,
                targets,
                this->proxy->get_context(),
                this->proxy->parameters.get<bool>(PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG),
                this->proxy->parameters.get<bool>(PatternMatchingQueryProxy::DISREGARD_IMPORTANCE_FLAG),
                this->proxy->parameters.get<bool>(PatternMatchingQueryProxy::UNIQUE_VALUE_FLAG),
                this->proxy->parameters.get<bool>(BaseQueryProxy::USE_LINK_TEMPLATE_CACHE));
            new_link_template->flat_pattern_flag = (this->count_open_expression == 1);
            this->element_stack.push(new_link_template);
        }
    } else if ((this->current_expression_type == AND) || (this->current_expression_type == OR)) {
        if (element_stack.size() >= 2) {
            shared_ptr<QueryElement> new_operator;
            // When using MeTTa to define a query, AND and OR operations always take 2 arguments.
            array<shared_ptr<QueryElement>, 2> clauses;
            vector<shared_ptr<QueryElement>> link_templates;
            for (int i = 1; i >= 0; i--) {
                LinkTemplate* link_template = dynamic_cast<LinkTemplate*>(element_stack.top().get());
                if (link_template != NULL) {
                    link_templates.push_back(element_stack.top());
                    link_template->build();
                    clauses[i] = link_template->get_source_element();
                } else {
                    if (element_stack.top()->is_operator) {
                        clauses[i] = element_stack.top();
                    } else {
                        Utils::error("All AND clauses are supposed to be LinkTemplate or Operator");
                        return;
                    }
                }
                element_stack.pop();
            }
            if (this->current_expression_type == AND) {
                LOG_DEBUG("Pushing AND");
                new_operator = make_shared<And<2>>(clauses, link_templates);
            } else {
                LOG_DEBUG("Pushing OR");
                new_operator = make_shared<Or<2>>(clauses, link_templates);
            }
            if (this->proxy->parameters.get<bool>(BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG)) {
                element_stack.push(make_shared<UniqueAssignmentFilter>(new_operator));
            } else {
                element_stack.push(new_operator);
            }
        } else {
            Utils::error("Invalid query expression: 'and' and 'or' require exactly two arguments.");
            return;
        }
    } else {
        Utils::error("Invalid query expression: unknown expression type");
        return;
    }

    this->current_expression_size = this->expression_size_stack.top() + 1;
    this->expression_size_stack.pop();

    if ((this->current_expression_type == LINK_TEMPLATE) &&
        (this->expression_type_stack.top() == LINK)) {
        LOG_DEBUG("Turning LINK into LINK_TEMPLATE");
        this->current_expression_type = LINK_TEMPLATE;
    } else {
        this->current_expression_type = this->expression_type_stack.top();
    }
    this->expression_type_stack.pop();
}
