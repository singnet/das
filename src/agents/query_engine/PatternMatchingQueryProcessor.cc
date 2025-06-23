#include "PatternMatchingQueryProcessor.h"

#include "And.h"
#include "AtomDBSingleton.h"
#include "LinkTemplate.h"
#include "LinkTemplate2.h"
#include "Or.h"
#include "PatternMatchingQueryProxy.h"
#include "ServiceBus.h"
#include "Sink.h"
#include "StoppableThread.h"
#include "Terminal.h"
#include "UniqueAssignmentFilter.h"

#define LOG_LEVEL INFO_LEVEL
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
    unsigned int bundle_count = 0;
    while ((answer = query_sink->input_buffer->pop_query_answer()) != NULL) {
        answer_count++;
        bundle_count++;
        if (!proxy->get_count_flag()) {
            answer_bundle.push_back(answer->tokenize());
        }
        if (proxy->get_attention_update_flag()) {
            update_attention_broker_single_answer(proxy, answer, joint_answer);
        }
        if (answer_bundle.size() >= MAX_BUNDLE_SIZE) {
            proxy->to_remote_peer(PatternMatchingQueryProxy::ANSWER_BUNDLE, answer_bundle);
            answer_bundle.clear();
            bundle_count = 0;
        }
        delete answer;
    }
    if (answer_bundle.size() > 0) {
        proxy->to_remote_peer(PatternMatchingQueryProxy::ANSWER_BUNDLE, answer_bundle);
    }
}

void PatternMatchingQueryProcessor::thread_process_one_query(
    shared_ptr<StoppableThread> monitor, shared_ptr<PatternMatchingQueryProxy> proxy) {
    try {
        if (proxy->args.size() < 2) {
            Utils::error("Syntax error in query command. Missing implicit parameters.");
        }
        unsigned int skip_arg = 0;
        proxy->set_context(proxy->args[skip_arg++]);
        proxy->set_unique_assignment_flag(proxy->args[skip_arg++] == "1");
        proxy->set_positive_importance_flag(proxy->args[skip_arg++] == "1");
        proxy->set_attention_update_flag(proxy->args[skip_arg++] == "1");
        proxy->set_count_flag(proxy->args[skip_arg++] == "1");
        proxy->query_tokens.insert(
            proxy->query_tokens.begin(), proxy->args.begin() + skip_arg, proxy->args.end());
        LOG_DEBUG("attention_update_flag: " << proxy->get_attention_update_flag());
        LOG_DEBUG("Setting up query tree");
        auto query_element_registry = make_unique<QueryElementRegistry>();
        shared_ptr<QueryElement> root_query_element =
            setup_query_tree(proxy, query_element_registry.get());
        set<string> joint_answer;  // used to stimulate attention broker
        string command = proxy->get_command();
        unsigned int sink_port_number;
        if (root_query_element == NULL) {
            Utils::error("Invalid empty query tree.");
        } else {
            if (command == ServiceBus::PATTERN_MATCHING_QUERY) {
                sink_port_number = PortPool::get_port();
                shared_ptr<Sink> query_sink = make_shared<Sink>(
                    root_query_element,
                    "Sink_" + proxy->peer_id() + "_" + std::to_string(proxy->get_serial()));
                unsigned int answer_count = 0;
                LOG_DEBUG("Processing QueryAnswer objects");
                while (!(query_sink->finished() || proxy->is_aborting())) {
                    process_query_answers(proxy, query_sink, joint_answer, answer_count);
                    Utils::sleep();
                }
                if (proxy->get_count_flag() && (!proxy->is_aborting())) {
                    LOG_DEBUG("Answering count_only query");
                    proxy->to_remote_peer(PatternMatchingQueryProxy::COUNT,
                                          {std::to_string(answer_count)});
                }
                Utils::sleep(500);
                proxy->to_remote_peer(PatternMatchingQueryProxy::FINISHED, {});
                if (proxy->get_attention_update_flag()) {
                    LOG_DEBUG("Updating AttentionBroker (stimulate)");
                    update_attention_broker_joint_answer(proxy, joint_answer);
                }
                Utils::sleep(500);
                LOG_INFO("Total processed answers: " << answer_count);
                query_sink->graceful_shutdown();
                PortPool::return_port(sink_port_number);
            } else {
                Utils::error("Invalid command " + command + " in PatternMatchingQueryProcessor");
            }
        }
    } catch (const std::runtime_error& exception) {
        proxy->raise_error_on_peer(exception.what());
    } catch (const std::exception& exception) {
        proxy->raise_error_on_peer(exception.what());
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
    shared_ptr<PatternMatchingQueryProxy> proxy, QueryElementRegistry* query_element_registry) {
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
        Utils::error("Parse error in query tokens");
        return shared_ptr<QueryElement>(NULL);
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
            element_stack.push(
                build_link_template(proxy, cursor, element_stack, query_element_registry));
        } else if (proxy->query_tokens[cursor] == "LINK_TEMPLATE2") {
            element_stack.push(build_link_template2(proxy, cursor, element_stack));
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
        Utils::error("Parse error in query tokens (trailing elements)");
        return shared_ptr<QueryElement>(NULL);
    }
    return element_stack.top();
}

#define BUILD_LINK_TEMPLATE(N)                                                              \
    {                                                                                       \
        array<shared_ptr<QueryElement>, N> targets;                                         \
        for (unsigned int i = 0; i < N; i++) {                                              \
            targets[i] = element_stack.top();                                               \
            element_stack.pop();                                                            \
        }                                                                                   \
        auto link_template = make_shared<LinkTemplate<N>>(proxy->query_tokens[cursor + 1],  \
                                                          move(targets),                    \
                                                          proxy->get_context(),             \
                                                          query_element_registry);          \
        link_template->set_positive_importance_flag(proxy->get_positive_importance_flag()); \
        return link_template;                                                               \
    }

shared_ptr<QueryElement> PatternMatchingQueryProcessor::build_link_template(
    shared_ptr<PatternMatchingQueryProxy> proxy,
    unsigned int cursor,
    stack<shared_ptr<QueryElement>>& element_stack,
    QueryElementRegistry* query_element_registry) {
    unsigned int arity = std::stoi(proxy->query_tokens[cursor + 2]);
    if (element_stack.size() < arity) {
        Utils::error(
            "PATTERN_MATCHING_QUERY message: parse error in tokens - too few arguments for "
            "LINK_TEMPLATE");
    }
    // clang-format off
    switch (arity) {
        case  1: BUILD_LINK_TEMPLATE(1);
        case  2: BUILD_LINK_TEMPLATE(2);
        case  3: BUILD_LINK_TEMPLATE(3);
        case  4: BUILD_LINK_TEMPLATE(4);
        case  5: BUILD_LINK_TEMPLATE(5);
        case  6: BUILD_LINK_TEMPLATE(6);
        case  7: BUILD_LINK_TEMPLATE(7);
        case  8: BUILD_LINK_TEMPLATE(8);
        case  9: BUILD_LINK_TEMPLATE(9);
        case 10: BUILD_LINK_TEMPLATE(10);
        // clang-format on
        default: {
            Utils::error("PATTERN_MATCHING_QUERY message: max supported arity for LINK_TEMPLATE: 10");
        }
    }
    return NULL;  // Just to avoid warnings. This is not actually reachable.
}

#define BUILD_LINK_TEMPLATE2(N)                                                               \
    {                                                                                         \
        array<shared_ptr<QueryElement>, N> targets;                                           \
        for (unsigned int i = 0; i < N; i++) {                                                \
            targets[i] = element_stack.top();                                                 \
            element_stack.pop();                                                              \
        }                                                                                     \
        return make_shared<LinkTemplate2<N>>(proxy->query_tokens[cursor + 1], move(targets)); \
    }

shared_ptr<QueryElement> PatternMatchingQueryProcessor::build_link_template2(
    shared_ptr<PatternMatchingQueryProxy> proxy,
    unsigned int cursor,
    stack<shared_ptr<QueryElement>>& element_stack) {
    unsigned int arity = std::stoi(proxy->query_tokens[cursor + 2]);
    if (element_stack.size() < arity) {
        Utils::error(
            "PATTERN_MATCHING_QUERY message: parse error in tokens - too few arguments for "
            "LINK_TEMPLATE2");
    }
    // clang-format off
    switch (arity) {
        case  1: BUILD_LINK_TEMPLATE2(1);
        case  2: BUILD_LINK_TEMPLATE2(2);
        case  3: BUILD_LINK_TEMPLATE2(3);
        case  4: BUILD_LINK_TEMPLATE2(4);
        case  5: BUILD_LINK_TEMPLATE2(5);
        case  6: BUILD_LINK_TEMPLATE2(6);
        case  7: BUILD_LINK_TEMPLATE2(7);
        case  8: BUILD_LINK_TEMPLATE2(8);
        case  9: BUILD_LINK_TEMPLATE2(9);
        case 10: BUILD_LINK_TEMPLATE2(10);
        // clang-format on
        default: {
            Utils::error("PATTERN_MATCHING_QUERY message: max supported arity for LINK_TEMPLATE2: 10");
        }
    }
    return NULL;  // Just to avoid warnings. This is not actually reachable.
}

#define BUILD_AND(N)                                \
    {                                               \
        array<shared_ptr<QueryElement>, N> clauses; \
        for (unsigned int i = 0; i < N; i++) {      \
            clauses[i] = element_stack.top();       \
            element_stack.pop();                    \
        }                                           \
        return make_shared<And<N>>(clauses);        \
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
    // clang-format off
    switch (num_clauses) {
        case  1: BUILD_AND(1)
        case  2: BUILD_AND(2)
        case  3: BUILD_AND(3)
        case  4: BUILD_AND(4)
        case  5: BUILD_AND(5)
        case  6: BUILD_AND(6)
        case  7: BUILD_AND(7)
        case  8: BUILD_AND(8)
        case  9: BUILD_AND(9)
        case 10: BUILD_AND(10)
        // clang-format on
        default: {
            Utils::error("PATTERN_MATCHING_QUERY message: max supported num_clauses for AND: 10");
        }
    }
    return NULL;  // Just to avoid warnings. This is not actually reachable.
}

#define BUILD_OR(N)                                 \
    {                                               \
        array<shared_ptr<QueryElement>, N> clauses; \
        for (unsigned int i = 0; i < N; i++) {      \
            clauses[i] = element_stack.top();       \
            element_stack.pop();                    \
        }                                           \
        return make_shared<Or<N>>(clauses);         \
    }

shared_ptr<QueryElement> PatternMatchingQueryProcessor::build_or(
    shared_ptr<PatternMatchingQueryProxy> proxy,
    unsigned int cursor,
    stack<shared_ptr<QueryElement>>& element_stack) {
    unsigned int num_clauses = std::stoi(proxy->query_tokens[cursor + 1]);
    if (element_stack.size() < num_clauses) {
        Utils::error("PATTERN_MATCHING_QUERY message: parse error in tokens - too few arguments for OR");
    }
    // clang-format off
    switch (num_clauses) {
        case  1: BUILD_OR(1)
        case  2: BUILD_OR(2)
        case  3: BUILD_OR(3)
        case  4: BUILD_OR(4)
        case  5: BUILD_OR(5)
        case  6: BUILD_OR(6)
        case  7: BUILD_OR(7)
        case  8: BUILD_OR(8)
        case  9: BUILD_OR(9)
        case 10: BUILD_OR(10)
        // clang-format on
        default: {
            Utils::error("PATTERN_MATCHING_QUERY message: max supported num_clauses for OR: 10");
        }
    }
    return NULL;  // Just to avoid warnings. This is not actually reachable.
}

#define BUILD_LINK(N)                                                                \
    {                                                                                \
        array<shared_ptr<QueryElement>, N> targets;                                  \
        for (unsigned int i = 0; i < N; i++) {                                       \
            targets[i] = element_stack.top();                                        \
            element_stack.pop();                                                     \
        }                                                                            \
        return make_shared<Link<N>>(proxy->query_tokens[cursor + 1], move(targets)); \
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
    // clang-format off
    switch (arity) {
        case  1: BUILD_LINK(1)
        case  2: BUILD_LINK(2)
        case  3: BUILD_LINK(3)
        case  4: BUILD_LINK(4)
        case  5: BUILD_LINK(5)
        case  6: BUILD_LINK(6)
        case  7: BUILD_LINK(7)
        case  8: BUILD_LINK(8)
        case  9: BUILD_LINK(9)
        case 10: BUILD_LINK(10)
        // clang-format on
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
