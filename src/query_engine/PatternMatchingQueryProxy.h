#pragma once

using namespace std;

namespace query_engine {

/**
 *
 */
class PatternMatchingQueryProxy : public BusCommandProxy {

public:

    PatternMatchingQueryProxy();
    ~PatternMatchingQueryProxy();

private:

    string context;
    bool update_attention_broker;
    vector<string> query_tokens;

};

} // namespace query_engine
