#pragma once

#include <string>
#include <vector>

using namespace std;

namespace link_creation_agent {
struct LinkCreationAgentRequest {
    vector<string> query;
    vector<string> link_template;
    int max_results = 1000;
    int repeat = 1;
    bool infinite = false;
    string context = "";
    bool update_attention_broker = false;
    bool importance_flag = true;
    // Internal fields
    string id = "";
    string original_id = "";
    bool is_running = false;  ///< Indicates if the request is currently being processed
    bool aborting = false;    ///< Indicates if the request is being aborted
    int processed = 0;        ///< Number of processed items
    bool completed = false;   ///< Indicates if the request has been completed
    long last_execution = 0;
    int current_interval;
};
}  // namespace link_creation_agent