#include "Terminal.h"

#define LOG_LEVEL INFO_LEVEL
#include "Link.h"
#include "Logger.h"
#include "Node.h"
#include "UntypedVariable.h"

using namespace query_element;

Terminal::Terminal() : QueryElement() {
    // Atom
    init();
    this->is_atom = true;
    this->handle = "UNINITIALIZED";
}

Terminal::Terminal(const string& type, const string& name) : QueryElement() {
    // Node
    init();
    this->is_node = true;
    this->type = type;
    this->name = name;
}

Terminal::Terminal(const string& type, const vector<shared_ptr<QueryElement>>& targets)
    : QueryElement() {
    // Link
    init();
    this->is_link = true;
    this->type = type;
    this->targets = targets;
}

Terminal::Terminal(const string& name) : QueryElement() {
    // Variable
    init();
    this->is_variable = true;
    this->name = name;
}

void Terminal::init() {
    this->is_variable = false;
    this->is_node = false;
    this->is_link = false;
    this->is_atom = false;
    this->is_terminal = true;
}

string Terminal::to_string() {
    if (this->is_atom) {
        return this->handle;
    } else if (this->is_node) {
        return "<" + this->type + ", " + this->name + ">";
    } else if (this->is_variable) {
        return this->name;
    } else if (this->is_link) {
        string answer = this->type + "[";
        for (auto target : this->targets) {
            answer += target->to_string();
            answer += ", ";
        }
        answer.pop_back();
        answer.pop_back();
        return answer;
    } else {
        Utils::error("Invalid terminal");
        return "";
    }
}
