#include "MettaParserActions.h"

#include <algorithm>
#include <array>
#include <vector>

#include "Link.h"
#include "Logger.h"
#include "MettaMapping.h"
#include "Node.h"
#include "UntypedVariable.h"

using namespace atoms;

// -------------------------------------------------------------------------------------------------
// Public methods

MettaParserActions::MettaParserActions() {
    this->current_expression_size = 0;
    this->current_expression_type = LINK;
    this->custom_attributes = {};
    this->first_value = false;
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
    } else if (name == MettaMapping::TRUTH_VALUE) {
        this->current_expression_type = STV;
    } else {
        if ((this->current_expression_type == AND) || (this->current_expression_type == OR)) {
            RAISE_ERROR("Invalid metta expression: AND/OR can't operate symbols.");
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
        RAISE_ERROR("Invalid metta expression: AND/OR can't operate literals.");
        return;
    } else if (this->current_expression_type == STV) {
        if (this->first_value) {
            this->first_value = false;
            PropertyValue& attr = this->custom_attributes.at("stv");
            if (pair<double, double>* stv = std::get_if<pair<double, double>>(&attr)) {
                stv->second = std::stod(value);
            }
        } else {
            this->first_value = true;
            this->custom_attributes["stv"] = make_pair(std::stod(value), 0.0);
        }
    } else {
        this->current_expression_size++;
        auto node = make_shared<Node>(MettaMapping::SYMBOL_NODE_TYPE, value);
        this->handle_to_atom[node->handle()] = node;
        this->handle_to_metta_expression[node->handle()] = value;
        this->element_stack.push(node);
    }
}

void MettaParserActions::literal(int value) {
    ParserActions::literal(value);
    if ((this->current_expression_type == AND) || (this->current_expression_type == OR)) {
        RAISE_ERROR("Invalid metta expression: AND/OR can't operate literals.");
        return;
    } else if (this->current_expression_type == STV) {
        if (this->first_value) {
            this->first_value = false;
            PropertyValue& attr = this->custom_attributes.at("stv");
            if (pair<double, double>* stv = std::get_if<pair<double, double>>(&attr)) {
                stv->second = static_cast<double>(value);
            }
        } else {
            this->first_value = true;
            this->custom_attributes["stv"] = make_pair(static_cast<double>(value), 0.0);
        }
    } else {
        this->current_expression_size++;
        auto node = make_shared<Node>(MettaMapping::SYMBOL_NODE_TYPE, std::to_string(value));
        this->handle_to_atom[node->handle()] = node;
        this->handle_to_metta_expression[node->handle()] = std::to_string(value);
        this->element_stack.push(node);
    }
}

void MettaParserActions::literal(float value) {
    ParserActions::literal(value);
    if ((this->current_expression_type == AND) || (this->current_expression_type == OR)) {
        RAISE_ERROR("Invalid metta expression: AND/OR can't operate literals.");
    } else if (this->current_expression_type == STV) {
        if (this->first_value) {
            this->first_value = false;
            PropertyValue& attr = this->custom_attributes.at("stv");
            if (pair<double, double>* stv = std::get_if<pair<double, double>>(&attr)) {
                stv->second = static_cast<double>(value);
            }
        } else {
            this->first_value = true;
            this->custom_attributes["stv"] = make_pair(static_cast<double>(value), 0.0);
        }
    } else {
        this->current_expression_size++;
        auto node = make_shared<Node>(MettaMapping::SYMBOL_NODE_TYPE, std::to_string(value));
        this->handle_to_atom[node->handle()] = node;
        this->handle_to_metta_expression[node->handle()] = std::to_string(value);
        this->element_stack.push(node);
    }
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
        RAISE_ERROR("Invalid metta expression: too few arguments for expression. Expected: " +
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
        auto link = make_shared<Link>(
            MettaMapping::EXPRESSION_LINK_TYPE, targets, toplevel, this->custom_attributes);
        this->handle_to_atom[link->handle()] = link;
        this->handle_to_metta_expression[link->handle()] = metta_string;
        this->element_stack.push(link);
        if (toplevel) this->metta_expression_handle = link->handle();
    } else if ((this->current_expression_type == AND) || (this->current_expression_type == OR)) {
        RAISE_ERROR("Invalid metta expression: AND/OR are not supported.");
        return;
    } else if (this->current_expression_type == STV) {
        LOG_DEBUG("STV expression");
        this->current_expression_size = this->expression_size_stack.top();
        this->expression_size_stack.pop();

        this->current_expression_type = this->expression_type_stack.top();
        this->expression_type_stack.pop();
        return;
    } else {
        RAISE_ERROR("Invalid metta expression: unknown expression type");
        return;
    }

    this->current_expression_size = this->expression_size_stack.top() + 1;
    this->expression_size_stack.pop();

    this->current_expression_type = this->expression_type_stack.top();
    this->expression_type_stack.pop();
}
