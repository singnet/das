#include "PatternMatchingQueryProcessor.h"
#include "PatternMatchingQueryProxy.h"
#include "ServiceBus.h"

using namespace atomdb;

// -------------------------------------------------------------------------------------------------
// Constructors and destructors

PatternMatchingQueryProcessor::PatternMatchingQueryProcessor() 
    : BusCommandProcessor({"XXXXXXXXX PATTERN_MATCHING_QUERY"}) {
    this->query_processor = NULL;
}

PatternMatchingQueryProcessor::~PatternMatchingQueryProcessor() {
    if (this->query_processor != NULL) {
        this->query_processor->join();
        delete this->query_processor;
    }
}

// -------------------------------------------------------------------------------------------------
// Public methods

shared_ptr<BusCommandProxy> factory_empty_proxy() {
    return shared_ptr<PatternMatchingQueryProxy> proxy(new PatternMatchingQueryProxy());
}

void run_command(shared_ptr<BusCommandProxy> proxy) {
    this->proxy = dynamic_pointer_cast<PatternMatchingQueryProxy>(proxy);
    setup_query_tree();
    this->query_processor = 
        new thread(&PatternMatchingQueryProcessor::thread_process_one_query, this);
    AQUI while true
}

// -------------------------------------------------------------------------------------------------
// Private methods

void PatternMatchingQueryProcessor::thread_process_one_query() {
    auto das_node = dynamic_pointer_cast<DASNode>(node);

    if (this->proxy->command == ServiceBus::PATTERN_MATCHING_QUERY || 
        this->proxy->command == ServiceBus::COUNTING_QUERY) {

        auto local_id = this->proxy->node_id() AQUI
        auto remote_id = this->requestor_id;

        vector<unique_ptr<QueryAnswerProcessor>> query_answer_processors;
        if (this->command == DASNode::PATTERN_MATCHING_QUERY) {
            query_answer_processors.push_back(make_unique<HandlesAnswerProcessor>(local_id, remote_id));
            if (this->update_attention_broker)
                query_answer_processors.push_back(make_unique<AttentionBrokerUpdater>(this->context));
        } else if (this->command == DASNode::COUNTING_QUERY) {
            query_answer_processors.push_back(make_unique<CountAnswerProcessor>(local_id, remote_id));
        }

        PatternMatchingQuery::remote_sinks_deleter.add(
            new RemoteSink<HandlesAnswer>(this->root_query_element, move(query_answer_processors)));
    } else {
        Utils::error("Invalid command " + this->command + " in PatternMatchingQueryProcessor");
    }
}

void PatternMatchingQueryProcessor::setup_query_tree() {

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
        Utils::error("PatternMatchingQuery message: parse error in query tokens");
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
            element_stack.push(build_link(proxy->query_tokens, cursor, element_stack));
        } else if (proxy->query_tokens[cursor] == "LINK_TEMPLATE") {
            element_stack.push(build_link_template(proxy->query_tokens, cursor, element_stack));
        } else if (proxy->query_tokens[cursor] == "AND") {
            element_stack.push(build_and(proxy->query_tokens, cursor, element_stack));
        } else if (proxy->query_tokens[cursor] == "OR") {
            element_stack.push(build_or(proxy->query_tokens, cursor, element_stack));
        } else {
            Utils::error(
                "Invalid token " + 
                proxy->query_tokens[cursor] + 
                " in PatternMatchingQuery message");
        }
        execution_stack.pop();
    }

    if (element_stack.size() != 1) {
        Utils::error("PatternMatchingQuery message: " +
            "parse error in query tokens (trailing elements)");
    }
    this->root_query_element = element_stack.top();
    element_stack.pop();
}
