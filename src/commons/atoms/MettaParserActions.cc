#include <algorithm>
#include <array>
#include <vector>

// clang-format off

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

#include "MettaParserActions.h"

#include "Link.h"
#include "Node.h"
#include "MettaMapping.h"
#include "UntypedVariable.h"

// clang-format on

using namespace atoms;

// -------------------------------------------------------------------------------------------------
// Public methods

MettaParserActions::MettaParserActions() {
    this->current_expression_size = 0;
    this->current_expression_type = LINK;
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
            Utils::error("Invalid metta expression: AND/OR can't operate symbols.");
            return;
        }
        this->current_expression_size++;
        auto node = make_shared<Node>(MettaMapping::SYMBOL_NODE_TYPE, name);
        this->handle_to_atom[node->handle()] = node;
        this->handle_to_metta_expression[node->handle()] = name;
        this->element_stack.push(node);
    }
}

void MettaParserActions::variable(const string& value) {
    ParserActions::variable(value);
    this->current_expression_size++;
    auto variable = make_shared<UntypedVariable>(value);
    this->handle_to_atom[variable->handle()] = variable;
    this->handle_to_metta_expression[variable->handle()] = value;
    this->element_stack.push(variable);
}

void MettaParserActions::literal(const string& value) {
    ParserActions::literal(value);
    if ((this->current_expression_type == AND) || (this->current_expression_type == OR)) {
        Utils::error("Invalid metta expression: AND/OR can't operate literals.");
        return;
    }
    this->current_expression_size++;
    auto node = make_shared<Node>(MettaMapping::SYMBOL_NODE_TYPE, value);
    this->handle_to_atom[node->handle()] = node;
    this->handle_to_metta_expression[node->handle()] = value;
    this->element_stack.push(node);
}

void MettaParserActions::literal(int value) {
    ParserActions::literal(value);
    if ((this->current_expression_type == AND) || (this->current_expression_type == OR)) {
        Utils::error("Invalid metta expression: AND/OR can't operate literals.");
        return;
    }
    this->current_expression_size++;
    auto node = make_shared<Node>(MettaMapping::SYMBOL_NODE_TYPE, std::to_string(value));
    this->handle_to_atom[node->handle()] = node;
    this->handle_to_metta_expression[node->handle()] = std::to_string(value);
    this->element_stack.push(node);
}

void MettaParserActions::literal(float value) {
    ParserActions::literal(value);
    if ((this->current_expression_type == AND) || (this->current_expression_type == OR)) {
        Utils::error("Invalid metta expression: AND/OR can't operate literals.");
        return;
    }
    this->current_expression_size++;
    auto node = make_shared<Node>(MettaMapping::SYMBOL_NODE_TYPE, std::to_string(value));
    this->handle_to_atom[node->handle()] = node;
    this->handle_to_metta_expression[node->handle()] = std::to_string(value);
    this->element_stack.push(node);
}

void MettaParserActions::expression_begin() {
    ParserActions::expression_begin();
    this->expression_size_stack.push(this->current_expression_size);
    this->expression_type_stack.push(this->current_expression_type);
    this->current_expression_size = 0;
    this->current_expression_type = LINK;
}

void MettaParserActions::expression_end(bool toplevel, const string& metta_string) {
    ParserActions::expression_end(toplevel, metta_string);
    this->metta_expressions.push_back(metta_string);
    unsigned int arity = this->current_expression_size;
    if (element_stack.size() < arity) {
        Utils::error("Invalid metta expression: too few arguments for expression. Expected: " +
                     std::to_string(arity) + " Stack size: " + std::to_string(element_stack.size()));
        return;
    }
    LOG_DEBUG("Arity: " + std::to_string(arity));
    if ((this->current_expression_type == LINK)) {
        vector<shared_ptr<Atom>> target_atoms;
        for (unsigned int i = 0; i < arity; i++) {
            target_atoms.push_back(element_stack.top());
            element_stack.pop();
        }
        reverse(target_atoms.begin(), target_atoms.end());

        vector<string> targets;
        for (const auto& atom : target_atoms) {
            targets.push_back(atom->handle());
        }

        LOG_DEBUG("Pushing LINK");
        auto link = make_shared<Link>(MettaMapping::EXPRESSION_LINK_TYPE, targets, toplevel);
        this->handle_to_atom[link->handle()] = link;
        this->handle_to_metta_expression[link->handle()] = metta_string;
        this->element_stack.push(link);
        if (toplevel) this->metta_expression_handle = link->handle();
    } else if ((this->current_expression_type == AND) || (this->current_expression_type == OR)) {
        Utils::error("Invalid metta expression: AND/OR are not supported.");
        return;
    } else {
        Utils::error("Invalid metta expression: unknown expression type");
        return;
    }

    this->current_expression_size = this->expression_size_stack.top() + 1;
    this->expression_size_stack.pop();

    this->current_expression_type = this->expression_type_stack.top();
    this->expression_type_stack.pop();
}
