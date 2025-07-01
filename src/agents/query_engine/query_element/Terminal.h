#pragma once

#include <array>
#include <string>

#include "Atom.h"
#include "AtomDBSingleton.h"
#include "QueryElement.h"
#include "expression_hasher.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace query_engine;
using namespace atomdb;

namespace query_element {

// -------------------------------------------------------------------------------------------------
// Abstract Terminal superclass

/**
 * A QueryElement which represents terminals (i.e. Nodes, Links and Variables) in the query tree.
 */
class Terminal : public QueryElement {
   protected:
    /**
     * Protected constructor.
     */
    Terminal() : QueryElement() {
        this->handle = shared_ptr<char>{};
        this->is_variable = false;
        this->is_terminal = true;  // overrides QueryElement default
    }

   public:
    /**
     * Destructor.
     */
    ~Terminal(){};

    /**
     * Empty implementation. There are no QueryNode element to setup.
     */
    void virtual setup_buffers() {}

    /**
     * Empty implementation. There are no QueryNode element or local thread to shut down.
     */
    void virtual graceful_shutdown() {}

    /**
     * Returns a string representation of this Terminal (mainly for debugging; not optimized to
     * production environment).
     */
    virtual string to_string() = 0;

    /**
     * A flag to indicate whether this Terminal is a Variable or not.
     */
    bool is_variable;

    /**
     * Handle of the terminal.
     */
    shared_ptr<char> handle;

    /**
     * Name of the terminal.
     *
     * Actually, only Nodes and Variables have names; Links' name is an empty string.
     */
    string name;
};

// -------------------------------------------------------------------------------------------------
// Node

/**
 * QueryElement which represents a node.
 */
class Node : public Terminal {
   public:
    /**
     * Constructor.
     *
     * @param type Type of the node.
     * @param name Name of the node.
     */
    Node(const string& type, const string& name) : Terminal() {
        this->type = type;
        this->name = name;
        this->handle = shared_ptr<char>(terminal_hash((char*) type.c_str(), (char*) name.c_str()),
                                        default_delete<char[]>());
        LOG_INFO("Node " << this->to_string());
    }

    /**
     * Returns a string representation of this Node (mainly for debugging; not optimized to
     * production environment).
     */
    string to_string() {
        return "<" + this->type + ", " + this->name + ", " + string(this->handle.get()) + ">";
    }

    /**
     * Type of this node.
     */
    string type;
};

// -------------------------------------------------------------------------------------------------
// Link

/**
 * QueryElement which represents a link.
 */
template <unsigned int ARITY>
class Link : public Terminal {
   public:
    /**
     * Constructor.
     *
     * @param type Type of the Link.
     * @params targets Array with targets of the Link. Targets are supposed to be
     *         handles (i.e. strings). No nesting of Nodes or other Links are allowed.
     */
    Link(const string& type, array<shared_ptr<QueryElement>, ARITY>&& targets) : Terminal() {
        this->name = "";
        this->type = type;
        this->targets = move(targets);
        this->arity = ARITY;
        char* handle_keys[ARITY + 1];
        handle_keys[0] = (char*) named_type_hash((char*) type.c_str());
        for (unsigned int i = 1; i < (ARITY + 1); i++) {
            if (this->targets[i - 1]->is_terminal &&
                !dynamic_pointer_cast<Terminal>(this->targets[i - 1])->is_variable) {
                handle_keys[i] = dynamic_pointer_cast<Terminal>(this->targets[i - 1])->handle.get();
            } else {
                Utils::error("Invalid Link definition");
            }
        }
        this->handle =
            shared_ptr<char>(composite_hash(handle_keys, ARITY + 1), default_delete<char[]>());
        free(handle_keys[0]);
        LOG_INFO("Link " << this->to_string());
    }

    /**
     * Returns a string representation of this Node (mainly for debugging; not optimized to
     * production environment).
     */
    string to_string() {
        string answer = "(" + this->type + ", [";
        for (unsigned int i = 0; i < this->arity; i++) {
            answer += dynamic_pointer_cast<Terminal>(this->targets[i])->to_string();
            if (i != (this->arity - 1)) {
                answer += ", ";
            }
        }
        answer += "], " + string(this->handle.get()) + ")";
        return answer;
    }

    /**
     * Type of the Link
     */
    string type;

    /**
     * Arity of the Link.
     */
    unsigned int arity = ARITY;

    /**
     * Targets of the Link.
     */
    array<shared_ptr<QueryElement>, ARITY> targets;
};

// -------------------------------------------------------------------------------------------------
// Variable

/**
 * QueryElement which represents a variable.
 */
class Variable : public Terminal {
   public:
    /**
     * Constructor.
     *
     * @param name Name of the Variable.
     */
    Variable(const string& name) : Terminal() {
        this->name = name;
        this->handle =
            shared_ptr<char>(strdup((char*) Atom::WILDCARD_STRING.c_str()), default_delete<char[]>());
        this->is_variable = true;
    }

    /**
     * Returns a string representation of this Variable (mainly for debugging; not optimized to
     * production environment).
     */
    string to_string() { return "$(" + this->name + ")"; }
};

}  // namespace query_element
