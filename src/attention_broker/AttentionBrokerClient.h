#pragma once

#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>

using namespace std;

#define DEFAULT_ATTENTION_BROKER_ADDRESS "localhost:37007"

namespace attention_broker {

/**
 *
 */
class AttentionBrokerClient {
   public:
    ~AttentionBrokerClient() {}

    static void set_server_address(const string& ip_port);
    static void correlate(const set<string>& handles, const string& context);
    static void stimulate(const map<string, unsigned int>& handle_count, const string& context);
    static void get_importance(const vector<string>& handles,
                               const string& context,
                               vector<float>& importances);

   private:
    static mutex api_mutex;
    static string SERVER_ADDRESS;
    static unsigned int MAX_GET_IMPORTANCE_BUNDLE_SIZE;
    AttentionBrokerClient() {}
};

}  // namespace attention_broker
