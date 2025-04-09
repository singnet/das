#include "PatternMatchingQueryProcessor.h"
#include "PatternMatchingQueryProxy.h"
#include "ServiceBus.h"
#include "AttentionBrokerUpdater.h"
#include "CountAnswerProcessor.h"
#include "HandlesAnswerProcessor.h"
#include "QueryAnswerProcessor.h"
#include "LinkTemplate.h"
#include "Or.h"
#include "And.h"
#include "Terminal.h"
#include "RemoteSink.h"
#include "RemoteIterator.h"

using namespace atomdb;

// -------------------------------------------------------------------------------------------------
// Constructors and destructors

PatternMatchingQueryProcessor::PatternMatchingQueryProcessor() 
    : BusCommandProcessor({ServiceBus::PATTERN_MATCHING_QUERY}) {
}

PatternMatchingQueryProcessor::~PatternMatchingQueryProcessor() {
    for (auto thread : this->query_threads) {
        thread->join();
        delete thread;
    }
}

// -------------------------------------------------------------------------------------------------
// Public methods

shared_ptr<BusCommandProxy> PatternMatchingQueryProcessor::factory_empty_proxy() {
    shared_ptr<PatternMatchingQueryProxy> proxy(new PatternMatchingQueryProxy());
    return proxy;
}

void PatternMatchingQueryProcessor::run_command(shared_ptr<BusCommandProxy> proxy) {
    lock_guard<mutex> semaphore(this->query_threads_mutex);
    auto query_proxy = dynamic_pointer_cast<PatternMatchingQueryProxy>(proxy);
    this->query_threads.push_back(
        new thread(&PatternMatchingQueryProcessor::thread_process_one_query, 
        this, 
        query_proxy));
}

// -------------------------------------------------------------------------------------------------
// Private methods

void PatternMatchingQueryProcessor::thread_process_one_query(shared_ptr<PatternMatchingQueryProxy> proxy) {

    if (proxy->args.size() < 2) {
        Utils::error("Syntax error in query command. Missing implicit parameters.");
    }
    proxy->context = proxy->args[0];
    proxy->update_attention_broker = (proxy->args[1] == "1");
    proxy->query_tokens.insert(proxy->query_tokens.begin(), proxy->args.begin() + 2, proxy->args.end());
    shared_ptr<QueryElement> root_query_element = setup_query_tree(proxy);
    string command = proxy->get_command();
    unsigned int remote_sink_port_number;
    if (command == ServiceBus::PATTERN_MATCHING_QUERY || 
        command == ServiceBus::COUNTING_QUERY) {
        remote_sink_port_number = PortPool::get_port();
        string id = proxy->my_id();
        string host = id.substr(0, id.find(":"));
        string local_id = host + ":" + to_string(remote_sink_port_number);
        string remote_id = proxy->peer_id();
        vector<unique_ptr<QueryAnswerProcessor>> query_answer_processors;
        if (command == ServiceBus::PATTERN_MATCHING_QUERY) {
            proxy->count_flag = false;
            cout << "XXXXX thread_process_one_query() HandlesAnswerProcessor local_id: " << local_id << " remote_id: " << remote_id << endl;
            query_answer_processors.push_back(make_unique<HandlesAnswerProcessor>(local_id, remote_id));
            if (proxy->update_attention_broker) {
                query_answer_processors.push_back(make_unique<AttentionBrokerUpdater>(proxy->context));
            }
        } else if (command == ServiceBus::COUNTING_QUERY) {
            proxy->count_flag = true;
            query_answer_processors.push_back(make_unique<CountAnswerProcessor>(local_id, remote_id));
        }
        cout << "XXXXX thread_process_one_query() building RemoteSink" << endl;
        RemoteSink<HandlesAnswer> sink(root_query_element, move(query_answer_processors));
        cout << "XXXXX thread_process_one_query() going to sleep and wait for query to finish" << endl;
        while (! (sink.is_work_done() || proxy->is_aborting())) {
            Utils::sleep();
        }
        cout << "XXXXX thread_process_one_query() query results delivered. Shutting down..." << endl;
        sink.graceful_shutdown();
        PortPool::return_port(remote_sink_port_number);
        cout << "XXXXX thread_process_one_query() Thread finished" << endl;
    } else {
        Utils::error("Invalid command " + command + " in PatternMatchingQueryProcessor");
    }
    // TODO add a call to join/delete/remove thread
}

shared_ptr<QueryElement> PatternMatchingQueryProcessor::setup_query_tree(shared_ptr<PatternMatchingQueryProxy> proxy) {

    stack<unsigned int> execution_stack;
    stack<shared_ptr<QueryElement>> element_stack;
    unsigned int cursor = 0;
    unsigned int tokens_count = proxy->query_tokens.size();

    while (cursor < tokens_count) {
        execution_stack.push(cursor);
        if ((proxy->query_tokens[cursor] == "VARIABLE") || 
            (proxy->query_tokens[cursor] == "AND") || 
            ((proxy->query_tokens[cursor] == "OR"))) {
            cursor += 2;
        } else {
            cursor += 3;
        }
    }
    if (cursor != tokens_count) {
        Utils::error("PATTERN_MATCHING_QUERY message: parse error in query tokens");
    }

    while (!execution_stack.empty()) {
        cursor = execution_stack.top();
        if (proxy->query_tokens[cursor] == "NODE") {
            element_stack.push(
                make_shared<Node>(proxy->query_tokens[cursor + 1], 
                proxy->query_tokens[cursor + 2]));
        } else if (proxy->query_tokens[cursor] == "VARIABLE") {
            element_stack.push(make_shared<Variable>(proxy->query_tokens[cursor + 1]));
        } else if (proxy->query_tokens[cursor] == "LINK") {
            element_stack.push(build_link(proxy, cursor, element_stack));
        } else if (proxy->query_tokens[cursor] == "LINK_TEMPLATE") {
            element_stack.push(build_link_template(proxy, cursor, element_stack));
        } else if (proxy->query_tokens[cursor] == "AND") {
            element_stack.push(build_and(proxy, cursor, element_stack));
        } else if (proxy->query_tokens[cursor] == "OR") {
            element_stack.push(build_or(proxy, cursor, element_stack));
        } else {
            Utils::error(
                "Invalid token " + 
                proxy->query_tokens[cursor] + 
                " in PATTERN_MATCHING_QUERY message");
        }
        execution_stack.pop();
    }

    if (element_stack.size() != 1) {
        Utils::error("PATTERN_MATCHING_QUERY message: "
            "parse error in query tokens (trailing elements)");
    }
    return element_stack.top();
}


shared_ptr<QueryElement> PatternMatchingQueryProcessor::build_link_template(
    shared_ptr<PatternMatchingQueryProxy> proxy,
    unsigned int cursor, 
    stack<shared_ptr<QueryElement>>& element_stack) {

    unsigned int arity = std::stoi(proxy->query_tokens[cursor + 2]);
    if (element_stack.size() < arity) {
        Utils::error(
            "PATTERN_MATCHING_QUERY message: parse error in tokens - too few arguments for LINK_TEMPLATE");
    }
    switch (arity) {
        // TODO: consider replacing each "case" below by a pre-processor macro call
        case 1: {
            array<shared_ptr<QueryElement>, 1> targets;
            for (unsigned int i = 0; i < 1; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<LinkTemplate<1>>(proxy->query_tokens[cursor + 1], targets, proxy->context);
        }
        case 2: {
            array<shared_ptr<QueryElement>, 2> targets;
            for (unsigned int i = 0; i < 2; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<LinkTemplate<2>>(proxy->query_tokens[cursor + 1], targets, proxy->context);
        }
        case 3: {
            array<shared_ptr<QueryElement>, 3> targets;
            for (unsigned int i = 0; i < 3; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<LinkTemplate<3>>(proxy->query_tokens[cursor + 1], targets, proxy->context);
        }
        case 4: {
            array<shared_ptr<QueryElement>, 4> targets;
            for (unsigned int i = 0; i < 4; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<LinkTemplate<4>>(proxy->query_tokens[cursor + 1], targets, proxy->context);
        }
        case 5: {
            array<shared_ptr<QueryElement>, 5> targets;
            for (unsigned int i = 0; i < 5; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<LinkTemplate<5>>(proxy->query_tokens[cursor + 1], targets, proxy->context);
        }
        case 6: {
            array<shared_ptr<QueryElement>, 6> targets;
            for (unsigned int i = 0; i < 6; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<LinkTemplate<6>>(proxy->query_tokens[cursor + 1], targets, proxy->context);
        }
        case 7: {
            array<shared_ptr<QueryElement>, 7> targets;
            for (unsigned int i = 0; i < 7; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<LinkTemplate<7>>(proxy->query_tokens[cursor + 1], targets, proxy->context);
        }
        case 8: {
            array<shared_ptr<QueryElement>, 8> targets;
            for (unsigned int i = 0; i < 8; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<LinkTemplate<8>>(proxy->query_tokens[cursor + 1], targets, proxy->context);
        }
        case 9: {
            array<shared_ptr<QueryElement>, 9> targets;
            for (unsigned int i = 0; i < 9; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<LinkTemplate<9>>(proxy->query_tokens[cursor + 1], targets, proxy->context);
        }
        case 10: {
            array<shared_ptr<QueryElement>, 10> targets;
            for (unsigned int i = 0; i < 10; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<LinkTemplate<10>>(proxy->query_tokens[cursor + 1], targets, proxy->context);
        }
        default: {
            Utils::error("PATTERN_MATCHING_QUERY message: max supported arity for LINK_TEMPLATE: 10");
        }
    }
    return NULL;  // Just to avoid warnings. This is not actually reachable.
}

shared_ptr<QueryElement> PatternMatchingQueryProcessor::build_and(
    shared_ptr<PatternMatchingQueryProxy> proxy,
    unsigned int cursor, 
    stack<shared_ptr<QueryElement>>& element_stack) {

    unsigned int num_clauses = std::stoi(proxy->query_tokens[cursor + 1]);
    if (element_stack.size() < num_clauses) {
        Utils::error("PATTERN_MATCHING_QUERY message: parse error in tokens - too few arguments for AND");
    }
    switch (num_clauses) {
        // TODO: consider replacing each "case" below by a pre-processor macro call
        case 1: {
            array<shared_ptr<QueryElement>, 1> clauses;
            for (unsigned int i = 0; i < 1; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<And<1>>(clauses);
        }
        case 2: {
            array<shared_ptr<QueryElement>, 2> clauses;
            for (unsigned int i = 0; i < 2; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<And<2>>(clauses);
        }
        case 3: {
            array<shared_ptr<QueryElement>, 3> clauses;
            for (unsigned int i = 0; i < 3; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<And<3>>(clauses);
        }
        case 4: {
            array<shared_ptr<QueryElement>, 4> clauses;
            for (unsigned int i = 0; i < 4; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<And<4>>(clauses);
        }
        case 5: {
            array<shared_ptr<QueryElement>, 5> clauses;
            for (unsigned int i = 0; i < 5; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<And<5>>(clauses);
        }
        case 6: {
            array<shared_ptr<QueryElement>, 6> clauses;
            for (unsigned int i = 0; i < 6; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<And<6>>(clauses);
        }
        case 7: {
            array<shared_ptr<QueryElement>, 7> clauses;
            for (unsigned int i = 0; i < 7; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<And<7>>(clauses);
        }
        case 8: {
            array<shared_ptr<QueryElement>, 8> clauses;
            for (unsigned int i = 0; i < 8; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<And<8>>(clauses);
        }
        case 9: {
            array<shared_ptr<QueryElement>, 9> clauses;
            for (unsigned int i = 0; i < 9; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<And<9>>(clauses);
        }
        case 10: {
            array<shared_ptr<QueryElement>, 10> clauses;
            for (unsigned int i = 0; i < 10; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<And<10>>(clauses);
        }
        default: {
            Utils::error("PATTERN_MATCHING_QUERY message: max supported num_clauses for AND: 10");
        }
    }
    return NULL;  // Just to avoid warnings. This is not actually reachable.
}

shared_ptr<QueryElement> PatternMatchingQueryProcessor::build_or(
    shared_ptr<PatternMatchingQueryProxy> proxy,
    unsigned int cursor,
    stack<shared_ptr<QueryElement>>& element_stack) {

    unsigned int num_clauses = std::stoi(proxy->query_tokens[cursor + 1]);
    if (element_stack.size() < num_clauses) {
        Utils::error("PATTERN_MATCHING_QUERY message: parse error in tokens - too few arguments for OR");
    }
    switch (num_clauses) {
        // TODO: consider replacing each "case" below by a pre-processor macro call
        case 1: {
            array<shared_ptr<QueryElement>, 1> clauses;
            for (unsigned int i = 0; i < 1; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<Or<1>>(clauses);
        }
        case 2: {
            array<shared_ptr<QueryElement>, 2> clauses;
            for (unsigned int i = 0; i < 2; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<Or<2>>(clauses);
        }
        case 3: {
            array<shared_ptr<QueryElement>, 3> clauses;
            for (unsigned int i = 0; i < 3; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<Or<3>>(clauses);
        }
        case 4: {
            array<shared_ptr<QueryElement>, 4> clauses;
            for (unsigned int i = 0; i < 4; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<Or<4>>(clauses);
        }
        case 5: {
            array<shared_ptr<QueryElement>, 5> clauses;
            for (unsigned int i = 0; i < 5; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<Or<5>>(clauses);
        }
        case 6: {
            array<shared_ptr<QueryElement>, 6> clauses;
            for (unsigned int i = 0; i < 6; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<Or<6>>(clauses);
        }
        case 7: {
            array<shared_ptr<QueryElement>, 7> clauses;
            for (unsigned int i = 0; i < 7; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<Or<7>>(clauses);
        }
        case 8: {
            array<shared_ptr<QueryElement>, 8> clauses;
            for (unsigned int i = 0; i < 8; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<Or<8>>(clauses);
        }
        case 9: {
            array<shared_ptr<QueryElement>, 9> clauses;
            for (unsigned int i = 0; i < 9; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<Or<9>>(clauses);
        }
        case 10: {
            array<shared_ptr<QueryElement>, 10> clauses;
            for (unsigned int i = 0; i < 10; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<Or<10>>(clauses);
        }
        default: {
            Utils::error("PATTERN_MATCHING_QUERY message: max supported num_clauses for OR: 10");
        }
    }
    return NULL;  // Just to avoid warnings. This is not actually reachable.
}

shared_ptr<QueryElement> PatternMatchingQueryProcessor::build_link(
    shared_ptr<PatternMatchingQueryProxy> proxy,
    unsigned int cursor, 
    stack<shared_ptr<QueryElement>>& element_stack) {

    unsigned int arity = std::stoi(proxy->query_tokens[cursor + 2]);
    if (element_stack.size() < arity) {
        Utils::error("PATTERN_MATCHING_QUERY message: parse error in tokens - too few arguments for LINK");
    }
    switch (arity) {
        // TODO: consider replacing each "case" below by a pre-processor macro call
        case 1: {
            array<shared_ptr<QueryElement>, 1> targets;
            for (unsigned int i = 0; i < 1; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<Link<1>>(proxy->query_tokens[cursor + 1], targets);
        }
        case 2: {
            array<shared_ptr<QueryElement>, 2> targets;
            for (unsigned int i = 0; i < 2; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<Link<2>>(proxy->query_tokens[cursor + 1], targets);
        }
        case 3: {
            array<shared_ptr<QueryElement>, 3> targets;
            for (unsigned int i = 0; i < 3; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<Link<3>>(proxy->query_tokens[cursor + 1], targets);
        }
        case 4: {
            array<shared_ptr<QueryElement>, 4> targets;
            for (unsigned int i = 0; i < 4; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<Link<4>>(proxy->query_tokens[cursor + 1], targets);
        }
        case 5: {
            array<shared_ptr<QueryElement>, 5> targets;
            for (unsigned int i = 0; i < 5; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<Link<5>>(proxy->query_tokens[cursor + 1], targets);
        }
        case 6: {
            array<shared_ptr<QueryElement>, 6> targets;
            for (unsigned int i = 0; i < 6; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<Link<6>>(proxy->query_tokens[cursor + 1], targets);
        }
        case 7: {
            array<shared_ptr<QueryElement>, 7> targets;
            for (unsigned int i = 0; i < 7; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<Link<7>>(proxy->query_tokens[cursor + 1], targets);
        }
        case 8: {
            array<shared_ptr<QueryElement>, 8> targets;
            for (unsigned int i = 0; i < 8; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<Link<8>>(proxy->query_tokens[cursor + 1], targets);
        }
        case 9: {
            array<shared_ptr<QueryElement>, 9> targets;
            for (unsigned int i = 0; i < 9; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<Link<9>>(proxy->query_tokens[cursor + 1], targets);
        }
        case 10: {
            array<shared_ptr<QueryElement>, 10> targets;
            for (unsigned int i = 0; i < 10; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<Link<10>>(proxy->query_tokens[cursor + 1], targets);
        }
        default: {
            Utils::error("PATTERN_MATCHING_QUERY message: max supported arity for LINK: 10");
        }
    }
    return NULL;  // Just to avoid warnings. This is not actually reachable.
}

