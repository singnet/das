#include "Terminal.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"
#include "Node.h"
#include "Link.h"
#include "UntypedVariable.h"

using namespace query_element;

Terminal::Terminal(const string& type, const string& name) : QueryElement() {
    // Node
    init();
    this->atom = make_shared<Node>(type, name);
    this->handle = this->atom->handle();
}

Terminal::Terminal(const string& type, const vector<shared_ptr<QueryElement>>& targets) : QueryElement() {
    // Link
    init();
    vector<string> target_handle;
    for (auto target: targets) {
        if (target->is_terminal && !dynamic_pointer_cast<Terminal>(target)->is_variable) {
            target_handle.push_back(dynamic_pointer_cast<Terminal>(target)->handle);
        } else {
            Utils::error("Invalid Link definition");
        }
    }
    this->atom = make_shared<Link>(type, target_handle);
    this->handle = this->atom->handle();
}

Terminal::Terminal(const string& name) : QueryElement() {
    // Variable
    init();
    this->atom = make_shared<UntypedVariable>(name);
    this->handle = this->atom->schema_handle();
    this->is_variable = true;
}

void Terminal::init() {
    this->atom = shared_ptr<Atom>(NULL);
    this->handle = "";
    this->is_variable = false;
    this->is_terminal = true;
}

string Terminal::to_string() {
    return atom->to_string();
}
