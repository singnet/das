#include "BusCommandProcessor.h"

using namespace service_bus;

// --------------------------------------------------------------------------------
// Public methods

BusCommandProcessor::BusCommandProcessor(const set<string>& commands) {
    this->commands = commands;
}

bool BusCommandProcessor::check_command(const string& command) {
    return (this->commands.find(command) != this->commands.end());
}
