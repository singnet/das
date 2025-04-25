/**
 * @file dummy_message.h
 * @brief Dummy message for link creation agent
 */
#pragma once

#include <iostream>
#include <vector>

#include "Logger.h"
#include "Message.h"

using namespace std;
using namespace distributed_algorithm_node;

/**
 * @brief Dummy message for unknown commands
 */
class DummyMessage : public Message {
   public:
    string command;
    vector<string> args;
    DummyMessage(string command, vector<string>& args) {
        this->command = command;
        this->args = args;
    }

    void act(shared_ptr<MessageFactory> node) { LOG_DEBUG(command); }
};
