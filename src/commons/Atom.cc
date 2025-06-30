#include "Atom.h"
#include "expression_hasher.h"

using namespace commons;

string Atom::UNDEFINED_TYPE = "__UNDEFINED_TYPE__";
string Atom::WILDCARD_STRING = "*";

// The minor memory leak below is disregarded for the sake of code simplicity
string Atom::WILDCARD_HANDLE = string(::compute_hash((char*) Atom::WILDCARD_STRING.c_str()));
