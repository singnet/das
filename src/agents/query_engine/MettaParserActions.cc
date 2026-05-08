#include <algorithm>
#include <array>
#include <vector>

// clang-format off

#include "Logger.h"

#include "And.h"
#include "LinkTemplate.h"
#include "MettaMapping.h"
#include "MettaParserActions.h"
#include "Or.h"
#include "Chain.h"
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
}

MettaParserActions::~MettaParserActions() {}

void MettaParserActions::symbol(const string& name) {
    ParserActions::symbol(name);
    if ((name == MettaMapping::AND_QUERY_OPERATOR) || (name == MettaMapping::OR_QUERY_OPERATOR) ||
        (name == MettaMapping::CHAIN_QUERY_OPERATOR)) {
        if (name == MettaMapping::AND_QUERY_OPERATOR) {
            this->current_expression_type = AND;
        } else if (name == MettaMapping::OR_QUERY_OPERATOR) {
            this->current_expression_type = OR;
        } else {
            this->current_expression_type = CHAIN;
        }
    } else {
        if ((this->current_expression_type == AND) || (this->current_expression_type == OR) ||
            (this->current_expression_type == CHAIN)) {
            RAISE_ERROR("Invalid query expression: AND/OR/CHAIN can't operate symbols.");
            return;
        }
        this->current_expression_size++;
        this->element_stack.push(make_shared<Terminal>(MettaMapping::SYMBOL_NODE_TYPE, name));
    }
}

void MettaParserActions::variable(const string& value) {
    ParserActions::variable(value);
    if ((this->current_expression_type == AND) || (this->current_expression_type == OR)) {
        RAISE_ERROR("Invalid query expression: AND/OR can't operate variables.");
        return;
    }
    if (this->current_expression_type == LINK) {
        this->current_expression_type = LINK_TEMPLATE;
    }
    this->current_expression_size++;
    this->element_stack.push(make_shared<Terminal>(value));
}

void MettaParserActions::literal(const string& value) {
    ParserActions::literal(value);
    if ((this->current_expression_type == AND) || (this->current_expression_type == OR)) {
        RAISE_ERROR("Invalid query expression: AND/OR can't operate literals.");
        return;
    }
    this->current_expression_size++;
    this->element_stack.push(make_shared<Terminal>(MettaMapping::SYMBOL_NODE_TYPE, value));
}

void MettaParserActions::literal(int value) {
    ParserActions::literal(value);
    if ((this->current_expression_type == AND) || (this->current_expression_type == OR)) {
        RAISE_ERROR("Invalid query expression: AND/OR can't operate literals.");
        return;
    }
    this->current_expression_size++;
    this->element_stack.push(
        make_shared<Terminal>(MettaMapping::SYMBOL_NODE_TYPE, std::to_string(value)));
}

void MettaParserActions::literal(float value) {
    ParserActions::literal(value);
    if ((this->current_expression_type == AND) || (this->current_expression_type == OR) ||
        (this->current_expression_type == CHAIN)) {
        RAISE_ERROR("Invalid query expression: AND/OR/CHAIN can't operate float literals.");
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
}

shared_ptr<Terminal> MettaParserActions::unstack_terminal(bool node_flag) {
    shared_ptr<Terminal> terminal = dynamic_pointer_cast<Terminal>(this->element_stack.top());
    if (terminal == nullptr) {
        RAISE_ERROR("Expecting Terminal. Got: " + this->element_stack.top()->to_string());
    } else {
        if (node_flag && (!terminal->is_node)) {
            RAISE_ERROR("Expecting Node. Got: " + this->element_stack.top()->to_string());
        }
    }
    this->element_stack.pop();
    return terminal;
}

void MettaParserActions::expression_end(bool toplevel, const string& metta_expression) {
    ParserActions::expression_end(toplevel, metta_expression);
    unsigned int arity = this->current_expression_size;
    if (this->element_stack.size() < arity) {
        RAISE_ERROR("Invalid query expression: too few arguments for expression. Expected: " +
                    std::to_string(arity) +
                    " Stack size: " + std::to_string(this->element_stack.size()));
        return;
    }
    LOG_DEBUG("Arity: " + std::to_string(arity));
    if ((this->current_expression_type == LINK) || (this->current_expression_type == LINK_TEMPLATE)) {
        vector<shared_ptr<QueryElement>> targets;
        for (unsigned int i = 0; i < arity; i++) {
            targets.push_back(this->element_stack.top());
            this->element_stack.pop();
        }
        reverse(targets.begin(), targets.end());
        if (this->current_expression_type == LINK) {
            if (toplevel) {
                RAISE_ERROR("Invalid query expression: a link is not a valid pattern matching query");
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
            this->element_stack.push(new_link_template);
        }
    } else if ((this->current_expression_type == AND) || (this->current_expression_type == OR)) {
        if (this->element_stack.size() >= 2) {
            shared_ptr<QueryElement> new_operator;
            // When using MeTTa to define a query, AND and OR operations always take 2 arguments.
            array<shared_ptr<QueryElement>, 2> clauses;
            vector<shared_ptr<QueryElement>> link_templates;
            for (int i = 1; i >= 0; i--) {
                LinkTemplate* link_template =
                    dynamic_cast<LinkTemplate*>(this->element_stack.top().get());
                if (link_template != NULL) {
                    link_templates.push_back(this->element_stack.top());
                    link_template->build();
                    clauses[i] = link_template->get_source_element();
                } else {
                    if (this->element_stack.top()->is_operator) {
                        clauses[i] = this->element_stack.top();
                    } else {
                        RAISE_ERROR("All AND clauses are supposed to be LinkTemplate or Operator");
                        return;
                    }
                }
                this->element_stack.pop();
            }
            if (this->current_expression_type == AND) {
                LOG_DEBUG("Pushing AND");
                new_operator = make_shared<And<2>>(clauses, link_templates);
            } else {
                LOG_DEBUG("Pushing OR");
                new_operator = make_shared<Or<2>>(clauses, link_templates);
            }
            if (this->proxy->parameters.get<bool>(BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG)) {
                this->element_stack.push(make_shared<UniqueAssignmentFilter>(new_operator));
            } else {
                this->element_stack.push(new_operator);
            }
        } else {
            RAISE_ERROR("Invalid query expression: 'and' and 'or' require exactly two arguments.");
            return;
        }
    } else if (this->current_expression_type == CHAIN) {
        if (this->element_stack.size() >= 6) {
            LOG_DEBUG("Building CHAIN operator...");
            array<shared_ptr<QueryElement>, 1> clauses;
            clauses[0] = this->element_stack.top();
            this->element_stack.pop();
            shared_ptr<LinkTemplate> link_template = dynamic_pointer_cast<LinkTemplate>(clauses[0]);
            if (link_template != nullptr) {
                link_template->build();
                clauses[0] = link_template->get_source_element();
            }
            LOG_DEBUG("Input: " + clauses[0]->to_string());
            shared_ptr<Terminal> target = unstack_terminal();
            shared_ptr<Terminal> source = unstack_terminal();
            shared_ptr<Terminal> cursor = unstack_terminal(true);
            unsigned int head_reference = Utils::string_to_uint(cursor->name);
            cursor = unstack_terminal(true);
            unsigned int tail_reference = Utils::string_to_uint(cursor->name);
            cursor = unstack_terminal(true);
            QueryAnswerElement link_selector;
            if (isdigit(static_cast<unsigned char>(cursor->name[0]))) {
                link_selector.set(Utils::string_to_uint(cursor->name));
                LOG_DEBUG("Link selector is handle index: " + cursor->name);
            } else {
                link_selector.set(cursor->name);
                LOG_DEBUG("Link selector is variable: " + cursor->name);
            }
            LOG_DEBUG("Tail reference: " + std::to_string(tail_reference));
            LOG_DEBUG("Head reference: " + std::to_string(head_reference));
            LOG_DEBUG("Source terminal: " + source->to_string());
            LOG_DEBUG("Target terminal: " + target->to_string());
            LOG_DEBUG("Source handle: " +
                      (source->is_variable ? "<variable>" : source->compute_handle()));
            LOG_DEBUG("Target handle: " +
                      (target->is_variable ? "<variable>" : target->compute_handle()));

            bool incomplete_flag =
                this->proxy->parameters.get<bool>(BaseQueryProxy::ALLOW_INCOMPLETE_CHAIN_PATH);
            Chain::SearchDirection search_direction;
            if (target->is_variable) {
                if (source->is_variable) {
                    RAISE_ERROR("Invalid CHAIN arguments. source and target are variables.");
                } else {
                    search_direction = Chain::FORWARD;
                    incomplete_flag = true;
                    LOG_DEBUG("Search direction: FORWARD");
                }
            } else {
                if (source->is_variable) {
                    search_direction = Chain::BACKWARD;
                    incomplete_flag = true;
                    LOG_DEBUG("Search direction: BACKWARD");
                } else {
                    search_direction = Chain::BOTH;
                    LOG_DEBUG("Search direction: FORWARD/BACKWARD");
                }
            }
            LOG_DEBUG(string("Allow incomplete paths: ") + (incomplete_flag ? "true" : "false"));

            auto chain_operator =
                make_shared<Chain>(clauses,
                                   link_template,
                                   source->is_variable ? source->name : source->compute_handle(),
                                   target->is_variable ? target->name : target->compute_handle(),
                                   search_direction,
                                   link_selector,
                                   tail_reference,
                                   head_reference,
                                   incomplete_flag);
            LOG_DEBUG("Building CHAIN operator... DONE");
            this->element_stack.push(chain_operator);
        } else {
            RAISE_ERROR("Invalid query expression: 'chain' requires exactly six arguments.");
            return;
        }
    } else {
        RAISE_ERROR("Invalid query expression: unknown expression type");
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
