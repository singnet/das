#ifndef _QUERY_ENGINE_DASNODE_H
#define _QUERY_ENGINE_DASNODE_H

#define DEBUG

#include <stack>

#include "HandlesAnswer.h"
#include "LazyWorkerDeleter.h"
#include "RemoteIterator.h"
#include "RemoteSink.h"
#include "Sink.h"
#include "StarNode.h"

using namespace std;
using namespace distributed_algorithm_node;
using namespace query_element;

namespace query_engine {

/**
 *
 */
class DASNode : public StarNode {
   public:
    static string PATTERN_MATCHING_QUERY;
    static string COUNTING_QUERY;

    DASNode(const string& node_id);
    DASNode(const string& node_id, const string& server_id);
    DASNode(const string& node_id,
            const string& server_id,
            unsigned int first_query_port,
            unsigned int last_query_port);
    ~DASNode();

    RemoteIterator<HandlesAnswer>* pattern_matcher_query(const vector<string>& tokens,
                                                         const string& context = "",
                                                         bool update_attention_broker = false);

    int count_query(const vector<string>& tokens,
                    const string& context = "",
                    bool update_attention_broker = false);

    string next_query_id();

    virtual shared_ptr<Message> message_factory(string& command, vector<string>& args);

   private:
    void initialize();

    string local_host;
    unsigned int next_query_port;
    unsigned int first_query_port = 60000;
    unsigned int last_query_port = 61999;
};

class PatternMatchingQuery : public Message {
   public:
    PatternMatchingQuery(string command, vector<string>& tokens);
    void act(shared_ptr<MessageFactory> node);

    static LazyWorkerDeleter<RemoteSink<HandlesAnswer>> remote_sinks_deleter;

   private:
    QueryElement* build_link_template(vector<string>& tokens,
                                      unsigned int cursor,
                                      stack<QueryElement*>& element_stack);

    QueryElement* build_and(vector<string>& tokens,
                            unsigned int cursor,
                            stack<QueryElement*>& element_stack);

    QueryElement* build_or(vector<string>& tokens,
                           unsigned int cursor,
                           stack<QueryElement*>& element_stack);

    QueryElement* build_link(vector<string>& tokens,
                             unsigned int cursor,
                             stack<QueryElement*>& element_stack);

    QueryElement* root_query_element;
    string requestor_id;
    string context;
    string command;
    bool update_attention_broker;
};

class CountingQuery : public Message {
   public:
    CountingQuery(string command, vector<string>& tokens);
    void act(shared_ptr<MessageFactory> node);

   private:
    unique_ptr<PatternMatchingQuery> pattern_matching_query;
};

}  // namespace query_engine

#endif  // _QUERY_ENGINE_DASNODE_H
