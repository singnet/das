#include <signal.h>

#include <iostream>
#include <string>

#include "AtomDBSingleton.h"
#include "AtomSpace.h"
#include "ContextBrokerProxy.h"
#include "QueryAnswer.h"
#include "ServiceBusSingleton.h"
#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

#define MAX_QUERY_ANSWERS ((unsigned int) 1000)

using namespace std;
using namespace query_engine;
using namespace service_bus;
using namespace atom_space;
using namespace atomdb;
using namespace context_broker;

void ctrl_c_handler(int) {
    cout << "Stopping query engine server..." << endl;
    cout << "Done." << endl;
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc < 6) {
        cerr << "Usage: " << argv[0]
             << " CLIENT_HOST:CLIENT_PORT SERVER_HOST:SERVER_PORT <START_PORT:END_PORT> "
                "UPDATE_ATTENTION_BROKER QUERY_TOKEN+ "
                "(hosts are supposed to be public IPs or known hostnames)"
             << endl;
        exit(1);
    }

    AtomDBSingleton::init();

    string client_id = string(argv[1]);
    string server_id = string(argv[2]);
    auto ports_range = Utils::parse_ports_range(argv[3]);
    bool update_attention_broker = (string(argv[4]) == "true" || string(argv[4]) == "1");
    if (update_attention_broker) {
        cerr << "Enforcing update_attention_broker=false regardless the passed parameter" << endl;
    }
    update_attention_broker = true;

    ServiceBusSingleton::init(client_id, server_id, ports_range.first, ports_range.second);

    bool USE_METTA_QUERY = true;

    // check if argv[4] is a number which is the max number of query answers
    // if not, set it to MAX_QUERY_ANSWERS
    int max_query_answers = 0;
    size_t tokens_start_position = 5;
    try {
        max_query_answers = stol(argv[5]);
        if (max_query_answers <= 0) {
            cout << "max_query_answers cannot be 0 or negative" << endl;
            exit(1);
        }
        tokens_start_position = 6;
    } catch (const exception& e) {
        max_query_answers = MAX_QUERY_ANSWERS;
    }
    cout << "Using max_query_answers: " << max_query_answers << endl;

    signal(SIGINT, &ctrl_c_handler);
    signal(SIGTERM, &ctrl_c_handler);
    vector<string> query;
    for (int i = tokens_start_position; i < argc; i++) {
        cout << "XXXXXXXXXX: " << argv[i] << endl;
        query.push_back(argv[i]);
    }

    LOG_INFO("Setting up context");

    string context_tag = "context";
    // Variables
    string sentence1 = "sentence1";
    string sentence2 = "sentence2";
    string sentence3 = "sentence3";
    string word1 = "word1";

    float RENT_RATE = 0.25;
    float SPREADING_RATE_LOWERBOUND = 0.50;
    float SPREADING_RATE_UPPERBOUND = 0.70;

    vector<string> context1 = {
        "LINK_TEMPLATE", "Expression", "3",
            "NODE", "Symbol", "Contains",
            "VARIABLE", "sentence1",
            "VARIABLE", "word1",
    };

    QueryAnswerElement sentence_link(sentence1);
    QueryAnswerElement word_link(word1);
    QueryAnswerElement contains_link(0);

    vector<pair<QueryAnswerElement, QueryAnswerElement>> determiner_schema = {
        {contains_link, sentence_link}, {sentence_link, word_link}};
    vector<QueryAnswerElement> stimulus_schema = {};

    // Use ContextBrokerProxy to create context
    shared_ptr<ContextBrokerProxy> context_proxy =
        make_shared<ContextBrokerProxy>(context_tag, context1, determiner_schema, stimulus_schema);

    context_proxy->parameters[ContextBrokerProxy::USE_CACHE] = (bool) true;
    context_proxy->parameters[ContextBrokerProxy::ENFORCE_CACHE_RECREATION] = (bool) false;

    context_proxy->parameters[ContextBrokerProxy::INITIAL_RENT_RATE] = (double) RENT_RATE;
    context_proxy->parameters[ContextBrokerProxy::INITIAL_SPREADING_RATE_LOWERBOUND] =
        (double) SPREADING_RATE_LOWERBOUND;
    context_proxy->parameters[ContextBrokerProxy::INITIAL_SPREADING_RATE_UPPERBOUND] =
        (double) SPREADING_RATE_UPPERBOUND;

    // Issue the ContextBrokerProxy to create context
    ServiceBusSingleton::get_instance()->issue_bus_command(context_proxy);

    // Wait for ContextBrokerProxy to finish context creation
    LOG_INFO("Waiting for context creation to finish...");
    while (!context_proxy->is_context_created()) {
        Utils::sleep();
    }

    string context_str = context_proxy->get_key();
    LOG_INFO("Context " + context_str + " was created");

    auto proxy = make_shared<PatternMatchingQueryProxy>(query, "context");
    proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = true;
    proxy->parameters[BaseQueryProxy::ATTENTION_UPDATE_FLAG] = update_attention_broker;
    proxy->parameters[BaseQueryProxy::USE_LINK_TEMPLATE_CACHE] = false;
    proxy->parameters[PatternMatchingQueryProxy::MAX_ANSWERS] = (unsigned int) 0;
    proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = false;
    proxy->parameters[BaseQueryProxy::USE_LINK_TEMPLATE_CACHE] = false;
    proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] = USE_METTA_QUERY;
    proxy->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = true;

    ServiceBusSingleton::get_instance()->issue_bus_command(proxy);

    shared_ptr<QueryAnswer> query_answer;
    int count = 0;
    while (!proxy->finished()) {
        if ((query_answer = proxy->pop()) == NULL) {
            Utils::sleep();
        } else {
            cout << query_answer->to_string() << endl;
            for (string handle : query_answer->handles) {
                cout << query_answer->metta_expression[handle] << endl;
            }
            if (++count == max_query_answers) {
                break;
            }
        }
    }
    cout << count << " answers" << endl;
    if (count == 0) {
        cout << "No match for query" << endl;
    }

    return 0;
}
