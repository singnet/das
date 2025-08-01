#pragma once

#include <string>
#include <vector>

using namespace std;
using namespace query_engine;

namespace link_creation_agent {
struct LinkCreationAgentRequest {
    vector<string> query;
    vector<string> link_template;
    int max_results = 1000;
    int repeat = 1;
    long last_execution = 0;
    int current_interval;
    bool infinite = false;
    string context = "";
    bool update_attention_broker = false;
    string id = "";
    bool is_running = false;  ///< Indicates if the request is currently being processed
};
}  // namespace link_creation_agent