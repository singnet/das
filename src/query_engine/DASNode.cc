#include "DASNode.h"

#include <array>

#include "AttentionBrokerUpdater.h"
#include "CountAnswerProcessor.h"
#include "HandlesAnswerProcessor.h"
#include "LinkTemplate.h"
#include "Or.h"
#include "QueryAnswerProcessor.h"
#include "RemoteSink.h"
#include "Terminal.h"

using namespace query_engine;

string DASNode::PATTERN_MATCHING_QUERY = "pattern_matching_query";
string DASNode::COUNTING_QUERY = "counting_query";

// -------------------------------------------------------------------------------------------------
// Constructors and destructors

DASNode::DASNode(const string& node_id) : StarNode(node_id) {
    initialize();
    // SERVER
}

DASNode::DASNode(const string& node_id, const string& server_id) : StarNode(node_id, server_id) {
    initialize();
    // CLIENT
}

DASNode::~DASNode() {}

void DASNode::initialize() {
    this->first_query_port = 60000;
    this->last_query_port = 61999;
    string id = this->node_id();
    this->local_host = id.substr(0, id.find(":"));
    if (this->is_server) {
        this->next_query_port = this->first_query_port;
    } else {
        this->next_query_port = (this->first_query_port + this->last_query_port) / 2;
    }
}

// -------------------------------------------------------------------------------------------------
// Public client API

RemoteIterator<HandlesAnswer>* DASNode::pattern_matcher_query(const vector<string>& tokens,
                                                              const string& context,
                                                              bool update_attention_broker) {
#ifdef DEBUG
    cout << "DASNode::pattern_matcher_query() BEGIN" << endl;
    cout << "DASNode::pattern_matcher_query() tokens.size(): " << tokens.size() << endl;
    cout << "DASNode::pattern_matcher_query() context: " << context << endl;
    cout << "DASNode::pattern_matcher_query() update_attention_broker: " << update_attention_broker
         << endl;
#endif
    if (this->is_server) {
        Utils::error("pattern_matcher_query() is not available in DASNode server.");
    }
    // TODO XXX change this when requestor is set in basic Message
    string query_id = next_query_id();
    vector<string> args = {query_id, context, std::to_string(update_attention_broker)};
    args.insert(args.end(), tokens.begin(), tokens.end());
    send(PATTERN_MATCHING_QUERY, args, this->server_id);
#ifdef DEBUG
    cout << "DASNode::pattern_matcher_query() END" << endl;
#endif
    return new RemoteIterator<HandlesAnswer>(query_id);
}

int DASNode::count_query(const vector<string>& tokens,
                         const string& context,
                         bool update_attention_broker) {
#ifdef DEBUG
    cout << "DASNode::count_query() BEGIN" << endl;
    cout << "DASNode::count_query() tokens.size(): " << tokens.size() << endl;
    cout << "DASNode::count_query() context: " << context << endl;
    cout << "DASNode::count_query() update_attention_broker: " << update_attention_broker << endl;
#endif
    if (this->is_server) {
        Utils::error("count_query() is not available in DASNode server.");
    }

    string query_id = next_query_id();
    vector<string> args = {query_id, context, std::to_string(update_attention_broker)};
    args.insert(args.end(), tokens.begin(), tokens.end());
    send(COUNTING_QUERY, args, this->server_id);

    int count = UNDEFINED_COUNT;
    CountAnswer* count_answer;
    auto count_iterator = make_unique<RemoteIterator<CountAnswer>>(query_id);
    while (not count_iterator->finished()) {
        if ((count_answer = dynamic_cast<CountAnswer*>(count_iterator->pop())) != nullptr) {
            count = count_answer->get_count();
            break;
        }
        Utils::sleep();
    }
#ifdef DEBUG
    cout << "DASNode::count_query() END" << endl;
#endif
    return count;
}

// -------------------------------------------------------------------------------------------------
// Public generic methods

string DASNode::next_query_id() {
    unsigned int port = this->next_query_port++;
    unsigned int limit;
    if (this->is_server) {
        limit = ((this->first_query_port + this->last_query_port) / 2) - 1;
        if (this->next_query_port > limit) {
            this->next_query_port = this->first_query_port;
        }
    } else {
        limit = this->last_query_port;
        if (this->next_query_port > limit) {
            this->next_query_port = (this->first_query_port + this->last_query_port) / 2;
        }
    }
#ifdef DEBUG
    cout << "DASNode::next_query_id(): " << this->local_host + ":" + std::to_string(port) << endl;
#endif
    return this->local_host + ":" + std::to_string(port);
}

// -------------------------------------------------------------------------------------------------
// Messages

shared_ptr<Message> DASNode::message_factory(string& command, vector<string>& args) {
    std::shared_ptr<Message> message = DistributedAlgorithmNode::message_factory(command, args);
    if (message) {
        return message;
    }
    if (command == DASNode::PATTERN_MATCHING_QUERY) {
        return std::shared_ptr<Message>(new PatternMatchingQuery(command, args));
    } else if (command == DASNode::COUNTING_QUERY) {
        return std::shared_ptr<Message>(new CountingQuery(command, args));
    }
    return std::shared_ptr<Message>{};
}

QueryElement* PatternMatchingQuery::build_link_template(vector<string>& tokens,
                                                        unsigned int cursor,
                                                        stack<QueryElement*>& element_stack) {
    unsigned int arity = std::stoi(tokens[cursor + 2]);
    if (element_stack.size() < arity) {
        Utils::error(
            "PatternMatchingQuery message: parse error in tokens - too few arguments for LINK_TEMPLATE");
    }
    switch (arity) {
        // TODO: consider replacing each "case" below by a pre-processor macro call
        case 1: {
            array<QueryElement*, 1> targets;
            for (unsigned int i = 0; i < 1; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return LinkTemplate<1>::get_pool().allocate(tokens[cursor + 1], targets, this->context);
        }
        case 2: {
            array<QueryElement*, 2> targets;
            for (unsigned int i = 0; i < 2; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return LinkTemplate<2>::get_pool().allocate(tokens[cursor + 1], targets, this->context);
        }
        case 3: {
            array<QueryElement*, 3> targets;
            for (unsigned int i = 0; i < 3; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return LinkTemplate<3>::get_pool().allocate(tokens[cursor + 1], targets, this->context);
        }
        case 4: {
            array<QueryElement*, 4> targets;
            for (unsigned int i = 0; i < 4; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return LinkTemplate<4>::get_pool().allocate(tokens[cursor + 1], targets, this->context);
        }
        case 5: {
            array<QueryElement*, 5> targets;
            for (unsigned int i = 0; i < 5; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return LinkTemplate<5>::get_pool().allocate(tokens[cursor + 1], targets, this->context);
        }
        case 6: {
            array<QueryElement*, 6> targets;
            for (unsigned int i = 0; i < 6; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return LinkTemplate<6>::get_pool().allocate(tokens[cursor + 1], targets, this->context);
        }
        case 7: {
            array<QueryElement*, 7> targets;
            for (unsigned int i = 0; i < 7; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return LinkTemplate<7>::get_pool().allocate(tokens[cursor + 1], targets, this->context);
        }
        case 8: {
            array<QueryElement*, 8> targets;
            for (unsigned int i = 0; i < 8; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return LinkTemplate<8>::get_pool().allocate(tokens[cursor + 1], targets, this->context);
        }
        case 9: {
            array<QueryElement*, 9> targets;
            for (unsigned int i = 0; i < 9; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return LinkTemplate<9>::get_pool().allocate(tokens[cursor + 1], targets, this->context);
        }
        case 10: {
            array<QueryElement*, 10> targets;
            for (unsigned int i = 0; i < 10; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return LinkTemplate<10>::get_pool().allocate(tokens[cursor + 1], targets, this->context);
        }
        default: {
            Utils::error("PatternMatchingQuery message: max supported arity for LINK_TEMPLATE: 10");
        }
    }
    return NULL;  // Just to avoid warnings. This is not actually reachable.
}

QueryElement* PatternMatchingQuery::build_and(vector<string>& tokens,
                                              unsigned int cursor,
                                              stack<QueryElement*>& element_stack) {
    unsigned int num_clauses = std::stoi(tokens[cursor + 1]);
    if (element_stack.size() < num_clauses) {
        Utils::error("PatternMatchingQuery message: parse error in tokens - too few arguments for AND");
    }
    switch (num_clauses) {
        // TODO: consider replacing each "case" below by a pre-processor macro call
        case 1: {
            array<QueryElement*, 1> clauses;
            for (unsigned int i = 0; i < 1; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return And<1>::get_pool().allocate(clauses);
        }
        case 2: {
            array<QueryElement*, 2> clauses;
            for (unsigned int i = 0; i < 2; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return And<2>::get_pool().allocate(clauses);
        }
        case 3: {
            array<QueryElement*, 3> clauses;
            for (unsigned int i = 0; i < 3; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return And<3>::get_pool().allocate(clauses);
        }
        case 4: {
            array<QueryElement*, 4> clauses;
            for (unsigned int i = 0; i < 4; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return And<4>::get_pool().allocate(clauses);
        }
        case 5: {
            array<QueryElement*, 5> clauses;
            for (unsigned int i = 0; i < 5; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return And<5>::get_pool().allocate(clauses);
        }
        case 6: {
            array<QueryElement*, 6> clauses;
            for (unsigned int i = 0; i < 6; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return And<6>::get_pool().allocate(clauses);
        }
        case 7: {
            array<QueryElement*, 7> clauses;
            for (unsigned int i = 0; i < 7; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return And<7>::get_pool().allocate(clauses);
        }
        case 8: {
            array<QueryElement*, 8> clauses;
            for (unsigned int i = 0; i < 8; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return And<8>::get_pool().allocate(clauses);
        }
        case 9: {
            array<QueryElement*, 9> clauses;
            for (unsigned int i = 0; i < 9; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return And<9>::get_pool().allocate(clauses);
        }
        case 10: {
            array<QueryElement*, 10> clauses;
            for (unsigned int i = 0; i < 10; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return And<10>::get_pool().allocate(clauses);
        }
        default: {
            Utils::error("PatternMatchingQuery message: max supported num_clauses for AND: 10");
        }
    }
    return NULL;  // Just to avoid warnings. This is not actually reachable.
}

QueryElement* PatternMatchingQuery::build_or(vector<string>& tokens,
                                             unsigned int cursor,
                                             stack<QueryElement*>& element_stack) {
    unsigned int num_clauses = std::stoi(tokens[cursor + 1]);
    if (element_stack.size() < num_clauses) {
        Utils::error("PatternMatchingQuery message: parse error in tokens - too few arguments for OR");
    }
    switch (num_clauses) {
        // TODO: consider replacing each "case" below by a pre-processor macro call
        case 1: {
            array<QueryElement*, 1> clauses;
            for (unsigned int i = 0; i < 1; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return Or<1>::get_pool().allocate(clauses);
        }
        case 2: {
            array<QueryElement*, 2> clauses;
            for (unsigned int i = 0; i < 2; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return Or<2>::get_pool().allocate(clauses);
        }
        case 3: {
            array<QueryElement*, 3> clauses;
            for (unsigned int i = 0; i < 3; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return Or<3>::get_pool().allocate(clauses);
        }
        case 4: {
            array<QueryElement*, 4> clauses;
            for (unsigned int i = 0; i < 4; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return Or<4>::get_pool().allocate(clauses);
        }
        case 5: {
            array<QueryElement*, 5> clauses;
            for (unsigned int i = 0; i < 5; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return Or<5>::get_pool().allocate(clauses);
        }
        case 6: {
            array<QueryElement*, 6> clauses;
            for (unsigned int i = 0; i < 6; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return Or<6>::get_pool().allocate(clauses);
        }
        case 7: {
            array<QueryElement*, 7> clauses;
            for (unsigned int i = 0; i < 7; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return Or<7>::get_pool().allocate(clauses);
        }
        case 8: {
            array<QueryElement*, 8> clauses;
            for (unsigned int i = 0; i < 8; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return Or<8>::get_pool().allocate(clauses);
        }
        case 9: {
            array<QueryElement*, 9> clauses;
            for (unsigned int i = 0; i < 9; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return Or<9>::get_pool().allocate(clauses);
        }
        case 10: {
            array<QueryElement*, 10> clauses;
            for (unsigned int i = 0; i < 10; i++) {
                clauses[i] = element_stack.top();
                element_stack.pop();
            }
            return Or<10>::get_pool().allocate(clauses);
        }
        default: {
            Utils::error("PatternMatchingQuery message: max supported num_clauses for OR: 10");
        }
    }
    return NULL;  // Just to avoid warnings. This is not actually reachable.
}

QueryElement* PatternMatchingQuery::build_link(vector<string>& tokens,
                                               unsigned int cursor,
                                               stack<QueryElement*>& element_stack) {
    unsigned int arity = std::stoi(tokens[cursor + 2]);
    if (element_stack.size() < arity) {
        Utils::error("PatternMatchingQuery message: parse error in tokens - too few arguments for LINK");
    }
    switch (arity) {
        // TODO: consider replacing each "case" below by a pre-processor macro call
        case 1: {
            array<QueryElement*, 1> targets;
            for (unsigned int i = 0; i < 1; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return Link<1>::get_pool().allocate(tokens[cursor + 1], targets);
        }
        case 2: {
            array<QueryElement*, 2> targets;
            for (unsigned int i = 0; i < 2; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return Link<2>::get_pool().allocate(tokens[cursor + 1], targets);
        }
        case 3: {
            array<QueryElement*, 3> targets;
            for (unsigned int i = 0; i < 3; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return Link<3>::get_pool().allocate(tokens[cursor + 1], targets);
        }
        case 4: {
            array<QueryElement*, 4> targets;
            for (unsigned int i = 0; i < 4; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return Link<4>::get_pool().allocate(tokens[cursor + 1], targets);
        }
        case 5: {
            array<QueryElement*, 5> targets;
            for (unsigned int i = 0; i < 5; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return Link<5>::get_pool().allocate(tokens[cursor + 1], targets);
        }
        case 6: {
            array<QueryElement*, 6> targets;
            for (unsigned int i = 0; i < 6; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return Link<6>::get_pool().allocate(tokens[cursor + 1], targets);
        }
        case 7: {
            array<QueryElement*, 7> targets;
            for (unsigned int i = 0; i < 7; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return Link<7>::get_pool().allocate(tokens[cursor + 1], targets);
        }
        case 8: {
            array<QueryElement*, 8> targets;
            for (unsigned int i = 0; i < 8; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return Link<8>::get_pool().allocate(tokens[cursor + 1], targets);
        }
        case 9: {
            array<QueryElement*, 9> targets;
            for (unsigned int i = 0; i < 9; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return Link<9>::get_pool().allocate(tokens[cursor + 1], targets);
        }
        case 10: {
            array<QueryElement*, 10> targets;
            for (unsigned int i = 0; i < 10; i++) {
                targets[i] = element_stack.top();
                element_stack.pop();
            }
            return Link<10>::get_pool().allocate(tokens[cursor + 1], targets);
        }
        default: {
            Utils::error("PatternMatchingQuery message: max supported arity for LINK: 10");
        }
    }
    return NULL;  // Just to avoid warnings. This is not actually reachable.
}

PatternMatchingQuery::PatternMatchingQuery(string command, vector<string>& tokens) {
#ifdef DEBUG
    cout << "PatternMatchingQuery::PatternMatchingQuery() BEGIN" << endl;
#endif

    stack<unsigned int> execution_stack;
    this->requestor_id = tokens[0];
    this->context = tokens[1];
    this->command = command;
    this->update_attention_broker = (tokens[2] == "1");
    unsigned int cursor = 3;  // TODO XXX: change this when requestor is set in basic Message
    unsigned int tokens_count = tokens.size();

#ifdef DEBUG
    cout << "PatternMatchingQuery::PatternMatchingQuery() tokens_count: " << tokens_count << endl;
#endif
    while (cursor < tokens_count) {
        execution_stack.push(cursor);
        if ((tokens[cursor] == "VARIABLE") || (tokens[cursor] == "AND") || ((tokens[cursor] == "OR"))) {
            cursor += 2;
        } else {
            cursor += 3;
        }
    }
    if (cursor != tokens_count) {
        Utils::error("PatternMatchingQuery message: parse error in tokens");
    }

    while (!execution_stack.empty()) {
        cursor = execution_stack.top();
        if (tokens[cursor] == "NODE") {
            this->element_stack.push(Node::get_pool().allocate(tokens[cursor + 1], tokens[cursor + 2]));
        } else if (tokens[cursor] == "VARIABLE") {
            this->element_stack.push(Variable::get_pool().allocate(tokens[cursor + 1]));
        } else if (tokens[cursor] == "LINK") {
            this->element_stack.push(build_link(tokens, cursor, this->element_stack));
        } else if (tokens[cursor] == "LINK_TEMPLATE") {
            this->element_stack.push(build_link_template(tokens, cursor, this->element_stack));
        } else if (tokens[cursor] == "AND") {
            this->element_stack.push(build_and(tokens, cursor, this->element_stack));
        } else if (tokens[cursor] == "OR") {
            this->element_stack.push(build_or(tokens, cursor, this->element_stack));
        } else {
            Utils::error("Invalid token " + tokens[cursor] + " in PatternMatchingQuery message");
        }
        execution_stack.pop();
    }

    if (this->element_stack.size() != 1) {
        Utils::error("PatternMatchingQuery message: parse error in tokens (trailing elements)");
    }
    this->root_query_element = this->element_stack.top();
    this->element_stack.pop();
#ifdef DEBUG
    cout << "PatternMatchingQuery::PatternMatchingQuery() END" << endl;
#endif
}

void PatternMatchingQuery::act(shared_ptr<MessageFactory> node) {
#ifdef DEBUG
    cout << "PatternMatchingQuery::act() BEGIN" << endl;
    cout << "PatternMatchingQuery::act() this->requestor_id: " << this->requestor_id << endl;
#endif
    auto das_node = dynamic_pointer_cast<DASNode>(node);

    if (this->command == DASNode::PATTERN_MATCHING_QUERY || this->command == DASNode::COUNTING_QUERY) {
        auto local_id = das_node->next_query_id();
        auto remote_id = this->requestor_id;

        vector<unique_ptr<QueryAnswerProcessor>> query_answer_processors;
        if (this->command == DASNode::PATTERN_MATCHING_QUERY) {
            query_answer_processors.push_back(make_unique<HandlesAnswerProcessor>(local_id, remote_id));
            if (this->update_attention_broker)
                query_answer_processors.push_back(make_unique<AttentionBrokerUpdater>(this->context));
        } else if (this->command == DASNode::COUNTING_QUERY) {
            query_answer_processors.push_back(make_unique<CountAnswerProcessor>(local_id, remote_id));
        }

        // TODO: eliminate this memory leak
        RemoteSink<HandlesAnswer>* remote_sink =
            new RemoteSink<HandlesAnswer>(this->root_query_element, move(query_answer_processors));
    } else {
        Utils::error("Invalid command " + this->command + " in PatternMatchingQuery message");
    }

#ifdef DEBUG
    cout << "PatternMatchingQuery::act() END" << endl;
#endif
}

CountingQuery::CountingQuery(string command, vector<string>& tokens) {
#ifdef DEBUG
    cout << "CountingQuery::CountingQuery() BEGIN" << endl;
#endif

    this->pattern_matching_query = make_unique<PatternMatchingQuery>(command, tokens);

#ifdef DEBUG
    cout << "CountingQuery::CountingQuery() END" << endl;
#endif
}

void CountingQuery::act(shared_ptr<MessageFactory> node) {
#ifdef DEBUG
    cout << "CountingQuery::act() BEGIN" << endl;
#endif

    this->pattern_matching_query->act(node);

#ifdef DEBUG
    cout << "CountingQuery::act() END" << endl;
#endif
}
