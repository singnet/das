#pragma once

#include <string>
#include <utility>

using namespace std;

namespace command_router {

/**
 * Splits a command line into COMMAND (first token) and ARG (remainder).
 */
pair<string, string> split_command_line(const string& line);

}  // namespace command_router
