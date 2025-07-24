#include "PatternMatchingQueryProcessor.h"

#include <grpcpp/grpcpp.h>

#include "And.h"
#include "Link.h"
#include "LinkSchema.h"
#include "LinkTemplate.h"
#include "Node.h"
#include "Or.h"
#include "PatternMatchingQueryProxy.h"
#include "ServiceBus.h"
#include "Sink.h"
#include "StoppableThread.h"
#include "Terminal.h"
#include "UniqueAssignmentFilter.h"
#include "UntypedVariable.h"
#include "attention_broker.grpc.pb.h"
#include "attention_broker.pb.h"

#define LOG_LEVEL DEBUG_LEVEL
#include "Logger.h"

using namespace atomdb;

string PatternMatchingQueryProcessor::AND = "AND";
string PatternMatchingQueryProcessor::OR = "OR";

// -------------------------------------------------------------------------------------------------
// Constructors and destructors

PatternMatchingQueryProcessor::PatternMatchingQueryProcessor()
    : BusCommandProcessor({ServiceBus::PATTERN_MATCHING_QUERY}) {
    this->atomdb = AtomDBSingleton::get_instance();
}

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
    set<string> single_answer;
    stack<string> execution_stack;

    for (string handle : answer->handles) {
        execution_stack.push(handle);
    }

    while (!execution_stack.empty()) {
        string handle = execution_stack.top();
        execution_stack.pop();
        // Updates single_answer (correlation)
        single_answer.insert(handle);
        // Updates joint answer (stimulation)
        joint_answer.insert(handle);
        // Gets targets and stack them
        shared_ptr<atomdb_api_types::HandleList> query_result = this->atomdb->query_for_targets(handle);
        if (query_result != NULL) {  // if handle is link
            unsigned int query_result_size = query_result->size();
            for (unsigned int i = 0; i < query_result_size; i++) {
                execution_stack.push(string(query_result->get_handle(i)));
            }
        } else {
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

void PatternMatchingQueryProcessor::recursive_metta_mapping(string handle, map<string, string>& table) {
    if (table.find(handle) == table.end()) {
        auto document = this->atomdb->get_atom_document(handle);
        if (document->contains("targets")) {
            // is link
            if (strcmp(document->get("named_type"), "Expression")) {
                Utils::error("Link type \"" + string(document->get("named_type")) + "\" can't be mapped to MeTTa");
                table[handle] = "";
                return;
            }
            unsigned int arity = document->get_size("targets");
            for (unsigned int i = 0; i < arity; i++) {
                recursive_metta_mapping(string(document->get("targets", i)), table);
            }
            string expression = "(";
            bool empty_flag = true;
            for (unsigned int i = 0; i < arity; i++) {
                expression += table[string(document->get("targets", i))];
                expression += " ";
                empty_flag = false;
            }
            if (! empty_flag) {
                expression.pop_back();
            }
            expression += ")";
            table[handle] = expression;
        } else {
            // is node
            if (strcmp(document->get("named_type"), "Symbol")) {
                Utils::error("Node type \"" + string(document->get("named_type")) + "\" can't be mapped to MeTTa");
                table[handle] = "";
                return;
            }
            table[handle] = document->get("name");
        }
    }
}

void PatternMatchingQueryProcessor::populate_metta_mapping(QueryAnswer* answer) {
    for (string handle: answer->handles) {
        recursive_metta_mapping(handle, answer->metta_expression);
    }
}

void PatternMatchingQueryProcessor::process_query_answers(
    shared_ptr<PatternMatchingQueryProxy> proxy,
    shared_ptr<Sink> query_sink,
    set<string>& joint_answer,  // used to stimulate attention broker
    unsigned int& answer_count) {
    QueryAnswer* answer;
    unsigned int max_answers = proxy->parameters.get<unsigned int>(BaseQueryProxy::MAX_ANSWERS);
    bool populate_metta = proxy->parameters.get<bool>(BaseQueryProxy::POPULATE_METTA_MAPPING);
    while ((answer = query_sink->input_buffer->pop_query_answer()) != NULL) {
        answer_count++;
        if (proxy->parameters.get<bool>(BaseQueryProxy::ATTENTION_UPDATE_FLAG)) {
            update_attention_broker_single_answer(proxy, answer, joint_answer);
        }
        if (!proxy->parameters.get<bool>(PatternMatchingQueryProxy::COUNT_FLAG)) {
            if (populate_metta) {
                populate_metta_mapping(answer);
            }
            proxy->push(shared_ptr<QueryAnswer>(answer));
        }
        if (answer_count == max_answers) {
            LOG_INFO("Limit number of answers reached: " << max_answers);
            proxy->flush_answer_bundle();
            proxy->abort({});
            return;
        }
    }
}

void PatternMatchingQueryProcessor::thread_process_one_query(
    shared_ptr<StoppableThread> monitor, shared_ptr<PatternMatchingQueryProxy> proxy) {
    STOP_WATCH_START(query_thread);
    try {
        proxy->untokenize(proxy->args);
        LOG_DEBUG("Setting up query tree");
        shared_ptr<QueryElement> root_query_element = setup_query_tree(proxy);
        set<string> joint_answer;  // used to stimulate attention broker
        string command = proxy->get_command();
        unsigned int sink_port_number;
        if (root_query_element == NULL) {
            Utils::error("Invalid empty query tree.");
        } else {
            if (command == ServiceBus::PATTERN_MATCHING_QUERY) {
                sink_port_number = PortPool::get_port();
                LinkTemplate* root_link_template = dynamic_cast<LinkTemplate*>(root_query_element.get());
                shared_ptr<Sink> query_sink;
                if (root_link_template != NULL) {
                    root_link_template->build();
                    query_sink = make_shared<Sink>(
                        root_link_template->get_source_element(),
                        "Sink_" + proxy->peer_id() + "_" + std::to_string(proxy->get_serial()));
                } else {
                    query_sink = make_shared<Sink>(
                        root_query_element,
                        "Sink_" + proxy->peer_id() + "_" + std::to_string(proxy->get_serial()));
                }
                unsigned int answer_count = 0;
                LOG_DEBUG("Processing QueryAnswer objects");
                while (!(query_sink->finished() || proxy->is_aborting())) {
                    process_query_answers(proxy, query_sink, joint_answer, answer_count);
                    Utils::sleep();
                }
                proxy->flush_answer_bundle();
                STOP_WATCH_FINISH(query_thread, "PatternMatchingQuery");
                if (proxy->parameters.get<bool>(PatternMatchingQueryProxy::COUNT_FLAG) &&
                    (!proxy->is_aborting())) {
                    LOG_DEBUG("Answering count_only query");
                    proxy->to_remote_peer(PatternMatchingQueryProxy::COUNT,
                                          {std::to_string(answer_count)});
                }
                Utils::sleep(500);
                proxy->query_processing_finished();
                if (proxy->parameters.get<bool>(BaseQueryProxy::ATTENTION_UPDATE_FLAG)) {
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
    shared_ptr<PatternMatchingQueryProxy> proxy) {
    stack<unsigned int> execution_stack;
    stack<shared_ptr<QueryElement>> element_stack;
    unsigned int cursor = 0;
    const vector<string> query_tokens = proxy->get_query_tokens();
    unsigned int tokens_count = query_tokens.size();

    while (cursor < tokens_count) {
        execution_stack.push(cursor);
        if ((query_tokens[cursor] == LinkSchema::LINK) ||
            (query_tokens[cursor] == LinkSchema::LINK_TEMPLATE) ||
            (query_tokens[cursor] == LinkSchema::NODE)) {
            cursor += 3;
        } else if ((query_tokens[cursor] == LinkSchema::UNTYPED_VARIABLE) ||
                   (query_tokens[cursor] == LinkSchema::ATOM) || (query_tokens[cursor] == AND) ||
                   (query_tokens[cursor] == OR)) {
            cursor += 2;
        } else {
            Utils::error("Invalid token in query: " + query_tokens[cursor]);
        }
    }

    if (cursor != tokens_count) {
        Utils::error("Parse error in query tokens");
        return shared_ptr<QueryElement>(NULL);
    }

    while (!execution_stack.empty()) {
        cursor = execution_stack.top();
        if (query_tokens[cursor] == LinkSchema::ATOM) {
            shared_ptr<Terminal> atom = make_shared<Terminal>();
            atom->handle = query_tokens[cursor + 1];
            element_stack.push(atom);
        } else if (query_tokens[cursor] == LinkSchema::NODE) {
            element_stack.push(
                make_shared<Terminal>(query_tokens[cursor + 1], query_tokens[cursor + 2]));
        } else if (query_tokens[cursor] == LinkSchema::UNTYPED_VARIABLE) {
            element_stack.push(make_shared<Terminal>(query_tokens[cursor + 1]));
        } else if (query_tokens[cursor] == LinkSchema::LINK) {
            element_stack.push(build_link(proxy, cursor, element_stack));
        } else if (query_tokens[cursor] == LinkSchema::LINK_TEMPLATE) {
            element_stack.push(build_link_template(proxy, cursor, element_stack));
        } else if (query_tokens[cursor] == AND) {
            element_stack.push(build_and(proxy, cursor, element_stack));
            if (proxy->parameters.get<bool>(BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG)) {
                element_stack.push(build_unique_assignment_filter(proxy, cursor, element_stack));
            }
        } else if (query_tokens[cursor] == OR) {
            element_stack.push(build_or(proxy, cursor, element_stack));
            if (proxy->parameters.get<bool>(BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG)) {
                element_stack.push(build_unique_assignment_filter(proxy, cursor, element_stack));
            }
        } else {
            Utils::error("Invalid token " + query_tokens[cursor] + " in PATTERN_MATCHING_QUERY message");
        }
        execution_stack.pop();
    }

    if (element_stack.size() != 1) {
        Utils::error("Parse error in query tokens (trailing elements)");
        return shared_ptr<QueryElement>(NULL);
    }
    return element_stack.top();
}

shared_ptr<QueryElement> PatternMatchingQueryProcessor::build_link_template(
    shared_ptr<PatternMatchingQueryProxy> proxy,
    unsigned int cursor,
    stack<shared_ptr<QueryElement>>& element_stack) {
    const vector<string> query_tokens = proxy->get_query_tokens();
    unsigned int arity = std::stoi(query_tokens[cursor + 2]);
    if (element_stack.size() < arity) {
        Utils::error(
            "PATTERN_MATCHING_QUERY message: parse error in tokens - too few arguments for "
            "LINK_TEMPLATE");
    }
    vector<shared_ptr<QueryElement>> targets;
    for (unsigned int i = 0; i < arity; i++) {
        targets.push_back(element_stack.top());
        element_stack.pop();
    }
    auto link_template = make_shared<LinkTemplate>(
        query_tokens[cursor + 1],
        targets,
        proxy->get_context(),
        proxy->parameters.get<bool>(PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG),
        proxy->parameters.get<bool>(BaseQueryProxy::USE_LINK_TEMPLATE_CACHE));
    return link_template;
}

#define BUILD_AND(N)                                                                              \
    {                                                                                             \
        vector<shared_ptr<QueryElement>> link_templates;                                          \
        array<shared_ptr<QueryElement>, N> clauses;                                               \
        for (unsigned int i = 0; i < N; i++) {                                                    \
            LinkTemplate* link_template = dynamic_cast<LinkTemplate*>(element_stack.top().get()); \
            if (link_template != NULL) {                                                          \
                link_templates.push_back(element_stack.top());                                    \
                link_template->build();                                                           \
                clauses[i] = link_template->get_source_element();                                 \
            } else {                                                                              \
                clauses[i] = element_stack.top();                                                 \
            }                                                                                     \
            element_stack.pop();                                                                  \
        }                                                                                         \
        return make_shared<And<N>>(clauses, link_templates);                                      \
    }

shared_ptr<QueryElement> PatternMatchingQueryProcessor::build_and(
    shared_ptr<PatternMatchingQueryProxy> proxy,
    unsigned int cursor,
    stack<shared_ptr<QueryElement>>& element_stack) {
    const vector<string> query_tokens = proxy->get_query_tokens();
    unsigned int num_clauses = std::stoi(query_tokens[cursor + 1]);
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

#define BUILD_OR(N)                                                                               \
    {                                                                                             \
        vector<shared_ptr<QueryElement>> link_templates;                                          \
        array<shared_ptr<QueryElement>, N> clauses;                                               \
        for (unsigned int i = 0; i < N; i++) {                                                    \
            LinkTemplate* link_template = dynamic_cast<LinkTemplate*>(element_stack.top().get()); \
            if (link_template != NULL) {                                                          \
                link_templates.push_back(element_stack.top());                                    \
                link_template->build();                                                           \
                clauses[i] = link_template->get_source_element();                                 \
            } else {                                                                              \
                clauses[i] = element_stack.top();                                                 \
            }                                                                                     \
            element_stack.pop();                                                                  \
        }                                                                                         \
        return make_shared<Or<N>>(clauses, link_templates);                                       \
    }

shared_ptr<QueryElement> PatternMatchingQueryProcessor::build_or(
    shared_ptr<PatternMatchingQueryProxy> proxy,
    unsigned int cursor,
    stack<shared_ptr<QueryElement>>& element_stack) {
    const vector<string> query_tokens = proxy->get_query_tokens();
    unsigned int num_clauses = std::stoi(query_tokens[cursor + 1]);
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

shared_ptr<QueryElement> PatternMatchingQueryProcessor::build_link(
    shared_ptr<PatternMatchingQueryProxy> proxy,
    unsigned int cursor,
    stack<shared_ptr<QueryElement>>& element_stack) {
    const vector<string> query_tokens = proxy->get_query_tokens();
    unsigned int arity = std::stoi(query_tokens[cursor + 2]);
    if (element_stack.size() < arity) {
        Utils::error(
            "PATTERN_MATCHING_QUERY message: parse error in tokens - too few arguments for LINK");
    }
    vector<shared_ptr<QueryElement>> targets;
    for (unsigned int i = 0; i < arity; i++) {
        targets.push_back(element_stack.top());
        element_stack.pop();
    }
    return make_shared<Terminal>(query_tokens[cursor + 1], targets);
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
