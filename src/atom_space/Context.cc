#include "Context.h"
#include "Hasher.h"

using namespace atom_space;
using namespace commons;

// -------------------------------------------------------------------------------------------------
// Public methods

Context::Context(const string& name) {
    this->name = name;
    this->key = Hasher::context_handle(name);
}

Context::~Context() {
}

const string& Context::get_name() {
    return name;
}

const string& Context::get_key() {
    return key;
}
