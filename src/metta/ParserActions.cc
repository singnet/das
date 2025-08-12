#include "ParserActions.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace metta;

// -------------------------------------------------------------------------------------------------
// Public methods

ParserActions::ParserActions() {}

ParserActions::~ParserActions() {}

void ParserActions::symbol(const string& name) { LOG_DEBUG("SYMBOL: <" + name + ">"); }

void ParserActions::variable(const string& value) { LOG_DEBUG("VARIABLE: <" + value + ">"); }

void ParserActions::literal(const string& value) { LOG_DEBUG("STRING_LITERAL: <" + value + ">"); }

void ParserActions::literal(int value) { LOG_DEBUG("INTEGER_LITERAL: <" + std::to_string(value) + ">"); }

void ParserActions::literal(float value) { LOG_DEBUG("FLOAT_LITERAL: <" + std::to_string(value) + ">"); }

void ParserActions::expression_begin() { LOG_DEBUG("BEGIN_EXPRESSION"); }

void ParserActions::expression_end(bool toplevel) {
    LOG_DEBUG(((toplevel ? "TOPLEVEL_" : "") + string("EXPRESSION")));
}
