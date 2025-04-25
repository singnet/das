#include "PatternMatchingQueryProcessor.h"

#include "And.h"
#include "AtomDBSingleton.h"
#include "LinkTemplate.h"
#include "Or.h"
#include "PatternMatchingQueryProxy.h"
#include "ServiceBus.h"
#include "Sink.h"
#include "StoppableThread.h"
#include "Terminal.h"
#include "UniqueAssignmentFilter.h"

#define LOG_LEVEL DEBUG_LEVEL
#include "Logger.h"

using namespace atomdb;

// -------------------------------------------------------------------------------------------------
// Constructors and destructors

PatternMatchingQueryProcessor::PatternMatchingQueryProcessor()
    : BusCommandProcessor({ServiceBus::PATTERN_MATCHING_QUERY}) {}

PatternMatchingQueryProcessor::~PatternMatchingQueryProcessor() {}

// -------------------------------------------------------------------------------------------------
// Public methods

shared_ptr<BusCommandProxy> PatternMatchingQueryProcessor::factory_empty_proxy() {
    shared_ptr<PatternMatchingQueryProxy> proxy(new PatternMatchingQueryProxy());
    return proxy;
}

void PatternMatchingQueryProcessor::run_command(shared_ptr<BusCommandProxy> proxy) {
    lock_guard<mutex> semaphore(this->query_threads_mutex);
    auto query_proxy = dynamic_pointer_cast<PatternMatchingQueryProxy>(proxy);
    string thread_id = "thread<" + proxy->my_id() + "_" + std::to_string(proxy->get_serial()) + ">";
    LOG_DEBUG("Starting new thread: " << thread_id << " to run command: <" << proxy->get_command()
                                      << ">");
    if (this->query_threads.find(thread_id) != this->query_threads.end()) {
        Utils::error("Invalid thread id: " + thread_id);
    } else {
        shared_ptr<StoppableThread> stoppable_thread = make_shared<StoppableThread>(thread_id);
        stoppable_thread->attach(new thread(&PatternMatchingQueryProcessor::thread_process_one_query,
                                            this,
                                            stoppable_thread,
                                            query_proxy));
        this->query_threads[thread_id] = stoppable_thread;
    }
}

// -------------------------------------------------------------------------------------------------
// Private methods

void PatternMatchingQueryProcessor::update_attention_broker_single_answer(
    shared_ptr<PatternMatchingQueryProxy> proxy, QueryAnswer* answer, set<string>& joint_answer) {
    shared_ptr<AtomDB> db = AtomDBSingleton::get_instance();
    set<string> single_answer;
    stack<string> execution_stack;

    for (unsigned int i = 0; i < answer->handles_size; i++) {
        string handle = string(answer->handles[i]);
        execution_stack.push(string(answer->handles[i]));
    }

    while (!execution_stack.empty()) {
        string handle = execution_stack.top();
        execution_stack.pop();
        // Updates single_answer (correlation)
        single_answer.insert(handle);
        // Updates joint answer (stimulation)
        joint_answer.insert(handle);
        // Gets targets and stack them
        shared_ptr<atomdb_api_types::HandleList> query_result =
            db->query_for_targets((char*) handle.c_str());
        if (query_result != NULL) {  // if handle is link
            unsigned int query_result_size = query_result->size();
            for (unsigned int i = 0; i < query_result_size; i++) {
                execution_stack.push(string(query_result->get_handle(i)));
            }
        }
    }

    auto stub = dasproto::AttentionBroker::NewStub(
        grpc::CreateChannel(ATTENTION_BROKER_ADDRESS, grpc::InsecureChannelCredentials()));

    dasproto::HandleList handle_list;  // GRPC command parameter
    dasproto::Ack ack;                 // GRPC command return
    handle_list.set_context(proxy->get_context());
    for (auto handle : single_answer) {
        handle_list.add_list(handle);
    }
    LOG_DEBUG("Calling AttentionBroker GRPC. Correlating " << single_answer.size() << " handles");
    stub->correlate(new grpc::ClientContext(), handle_list, &ack);
    if (ack.msg() != "CORRELATE") {
        Utils::error("Failed GRPC command: AttentionBroker::correlate()");
    }
}

void PatternMatchingQueryProcessor::update_attention_broker_joint_answer(
    shared_ptr<PatternMatchingQueryProxy> proxy, set<string>& joint_answer) {
    auto stub = dasproto::AttentionBroker::NewStub(
        grpc::CreateChannel(ATTENTION_BROKER_ADDRESS, grpc::InsecureChannelCredentials()));

    dasproto::HandleCount handle_count;  // GRPC command parameter
    dasproto::Ack ack;                   // GRPC command return
    handle_count.set_context(proxy->get_context());
    for (auto handle : joint_answer) {
        (*handle_count.mutable_map())[handle] = 1;
    }
    (*handle_count.mutable_map())["SUM"] = joint_answer.size();
    LOG_DEBUG("Calling AttentionBroker GRPC. Stimulating " << joint_answer.size() << " handles");
    stub->stimulate(new grpc::ClientContext(), handle_count, &ack);
    if (ack.msg() != "STIMULATE") {
        Utils::error("Failed GRPC command: AttentionBroker::stimulate()");
    }
}

void PatternMatchingQueryProcessor::process_query_answers(
    shared_ptr<PatternMatchingQueryProxy> proxy,
    shared_ptr<Sink> query_sink,
    set<string>& joint_answer,  // used to stimulate attention broker
    unsigned int& answer_count) {
    vector<string> answer_bundle;
    QueryAnswer* answer;
    while ((answer = query_sink->input_buffer->pop_query_answer()) != NULL) {
        answer_count++;
        if (!proxy->get_count_flag()) {
            answer_bundle.push_back(answer->tokenize());
        }
        if (proxy->get_attention_update_flag()) {
            update_attention_broker_single_answer(proxy, answer, joint_answer);
        }
        delete answer;
    }
    if (answer_bundle.size() > 0) {
        proxy->to_remote_peer(PatternMatchingQueryProxy::ANSWER_BUNDLE, answer_bundle);
    }
}

void PatternMatchingQueryProcessor::thread_process_one_query(
    shared_ptr<StoppableThread> monitor, shared_ptr<PatternMatchingQueryProxy> proxy) {
    if (proxy->args.size() < 2) {
        Utils::error("Syntax error in query command. Missing implicit parameters.");
    }
    unsigned int skip_arg = 0;
    proxy->set_context(proxy->args[skip_arg++]);
    proxy->set_unique_assignment_flag(proxy->args[skip_arg++] == "1");
    proxy->set_attention_update_flag(proxy->args[skip_arg++] == "1");
    proxy->set_count_flag(proxy->args[skip_arg++] == "1");
    proxy->query_tokens.insert(
        proxy->query_tokens.begin(), proxy->args.begin() + skip_arg, proxy->args.end());
    LOG_DEBUG("Setting up query tree");
    shared_ptr<QueryElement> root_query_element = setup_query_tree(proxy);
    set<string> joint_answer;  // used to stimulate attention broker
    string command = proxy->get_command();
    unsigned int sink_port_number;
    if (command == ServiceBus::PATTERN_MATCHING_QUERY) {
        sink_port_number = PortPool::get_port();
        shared_ptr<Sink> query_sink = make_shared<Sink>(
            root_query_element, "Sink_" + proxy->peer_id() + "_" + std::to_string(proxy->get_serial()));
        unsigned int answer_count = 0;
        LOG_DEBUG("Processing QueryAnswer objects");
        while (!(query_sink->finished() || proxy->is_aborting())) {
            process_query_answers(proxy, query_sink, joint_answer, answer_count);
            Utils::sleep();
        }
        if (proxy->get_count_flag() && (!proxy->is_aborting())) {
            LOG_DEBUG("Answering count_only query");
            proxy->to_remote_peer(PatternMatchingQueryProxy::COUNT, {std::to_string(answer_count)});
        }
        proxy->to_remote_peer(PatternMatchingQueryProxy::FINISHED, {});
        if (proxy->get_attention_update_flag()) {
            LOG_DEBUG("Updating AttentionBroker (stimulate)");
            update_attention_broker_joint_answer(proxy, joint_answer);
        }
        Utils::sleep(500);
        query_sink->graceful_shutdown();
        PortPool::return_port(sink_port_number);
    } else {
        Utils::error("Invalid command " + command + " in PatternMatchingQueryProcessor");
    }
    LOG_DEBUG("Command finished: <" << proxy->get_command() << ">");
    // TODO add a call to remove_query_thread(monitor->get_id());
}

void PatternMatchingQueryProcessor::remove_query_thread(const string& stoppable_thread_id) {
    lock_guard<mutex> semaphore(this->query_threads_mutex);
    this->query_threads.erase(this->query_threads.find(stoppable_thread_id));
}

// -------------------------------------------------------------------------------------------------
// Private methods - query tree building

shared_ptr<QueryElement> PatternMatchingQueryProcessor::setup_query_tree(
    shared_ptr<PatternMatchingQueryProxy> proxy) {
    stack<unsigned int> execution_stack;
    stack<shared_ptr<QueryElement>> element_stack;
    unsigned int cursor = 0;
    unsigned int tokens_count = proxy->query_tokens.size();

    while (cursor < tokens_count) {
        execution_stack.push(cursor);
        if ((proxy->query_tokens[cursor] == "VARIABLE") || (proxy->query_tokens[cursor] == "AND") ||
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
                make_shared<Node>(proxy->query_tokens[cursor + 1], proxy->query_tokens[cursor + 2]));
        } else if (proxy->query_tokens[cursor] == "VARIABLE") {
            element_stack.push(make_shared<Variable>(proxy->query_tokens[cursor + 1]));
        } else if (proxy->query_tokens[cursor] == "LINK") {
            element_stack.push(build_link(proxy, cursor, element_stack));
        } else if (proxy->query_tokens[cursor] == "LINK_TEMPLATE") {
            element_stack.push(build_link_template(proxy, cursor, element_stack));
        } else if (proxy->query_tokens[cursor] == "AND") {
            element_stack.push(build_and(proxy, cursor, element_stack));
            if (proxy->get_unique_assignment_flag()) {
                element_stack.push(build_unique_assignment_filter(proxy, cursor, element_stack));
            }
        } else if (proxy->query_tokens[cursor] == "OR") {
            element_stack.push(build_or(proxy, cursor, element_stack));
            if (proxy->get_unique_assignment_flag()) {
                element_stack.push(build_unique_assignment_filter(proxy, cursor, element_stack));
            }
        } else {
            Utils::error("Invalid token " + proxy->query_tokens[cursor] +
                         " in PATTERN_MATCHING_QUERY message");
        }
        execution_stack.pop();
    }

    if (element_stack.size() != 1) {
        Utils::error(
            "PATTERN_MATCHING_QUERY message: "
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
            "PATTERN_MATCHING_QUERY message: parse error in tokens - too few arguments for "
            "LINK_TEMPLATE");
    }
    switch (arity) {
        // TODO: consider replacing each "case" below by a pre-processor macro call
        case 1: {
            array<shared_ptr<QueryElement>, 1> targets;
            for (unsigned int i = 0; i < 1; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<LinkTemplate<1>>(
                proxy->query_tokens[cursor + 1], targets, proxy->get_context());
        }
        case 2: {
            array<shared_ptr<QueryElement>, 2> targets;
            for (unsigned int i = 0; i < 2; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<LinkTemplate<2>>(
                proxy->query_tokens[cursor + 1], targets, proxy->get_context());
        }
        case 3: {
            array<shared_ptr<QueryElement>, 3> targets;
            for (unsigned int i = 0; i < 3; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<LinkTemplate<3>>(
                proxy->query_tokens[cursor + 1], targets, proxy->get_context());
        }
        case 4: {
            array<shared_ptr<QueryElement>, 4> targets;
            for (unsigned int i = 0; i < 4; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<LinkTemplate<4>>(
                proxy->query_tokens[cursor + 1], targets, proxy->get_context());
        }
        case 5: {
            array<shared_ptr<QueryElement>, 5> targets;
            for (unsigned int i = 0; i < 5; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<LinkTemplate<5>>(
                proxy->query_tokens[cursor + 1], targets, proxy->get_context());
        }
        case 6: {
            array<shared_ptr<QueryElement>, 6> targets;
            for (unsigned int i = 0; i < 6; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<LinkTemplate<6>>(
                proxy->query_tokens[cursor + 1], targets, proxy->get_context());
        }
        case 7: {
            array<shared_ptr<QueryElement>, 7> targets;
            for (unsigned int i = 0; i < 7; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<LinkTemplate<7>>(
                proxy->query_tokens[cursor + 1], targets, proxy->get_context());
        }
        case 8: {
            array<shared_ptr<QueryElement>, 8> targets;
            for (unsigned int i = 0; i < 8; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<LinkTemplate<8>>(
                proxy->query_tokens[cursor + 1], targets, proxy->get_context());
        }
        case 9: {
            array<shared_ptr<QueryElement>, 9> targets;
            for (unsigned int i = 0; i < 9; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<LinkTemplate<9>>(
                proxy->query_tokens[cursor + 1], targets, proxy->get_context());
        }
        case 10: {
            array<shared_ptr<QueryElement>, 10> targets;
            for (unsigned int i = 0; i < 10; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return make_shared<LinkTemplate<10>>(
                proxy->query_tokens[cursor + 1], targets, proxy->get_context());
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
        Utils::error(
            "PATTERN_MATCHING_QUERY message: parse error in tokens - too few arguments for AND");
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
        Utils::error(
            "PATTERN_MATCHING_QUERY message: parse error in tokens - too few arguments for LINK");
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

shared_ptr<QueryElement> PatternMatchingQueryProcessor::build_unique_assignment_filter(
    shared_ptr<PatternMatchingQueryProxy> proxy,
    unsigned int cursor,
    stack<shared_ptr<QueryElement>>& element_stack) {
    if (element_stack.size() < 1) {
        Utils::error(
            "PATTERN_MATCHING_QUERY message: parse error in tokens - too few arguments for "
            "UniqueAssignmentFilter");
    }

    shared_ptr<QueryElement> input = element_stack.top();
    element_stack.pop();
    return make_shared<UniqueAssignmentFilter>(input);
}
