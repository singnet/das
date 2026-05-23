#include "CommandLineParser.h"

#include "Utils.h"

using namespace command_router;
using namespace commons;

pair<string, string> command_router::split_command_line(const string& line) {
    size_t pos = line.find(' ');
    if (pos == string::npos) {
        RAISE_ERROR("Invalid command line (expected '<COMMAND> <ARG>'): " + line);
    }
    string command = line.substr(0, pos);
    string arg = line.substr(pos + 1);
    Utils::trim(command);
    Utils::trim(arg);
    if (command.empty() || arg.empty()) {
        RAISE_ERROR("Invalid command line (empty COMMAND or ARG): " + line);
    }
    return {command, arg};
}
