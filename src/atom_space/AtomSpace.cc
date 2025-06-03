#include "AtomSpace.h"

namespace atomspace {

const Atom& AtomSpace::get_atom(const char* handle, Scope scope) {}

const Link& AtomSpace::get_link(const std::string& type,
                                const std::vector<std::string>& targets,
                                Scope scope) {}

const Node& AtomSpace::get_node(const std::string& type, const std::string& name, Scope scope) {}

std::shared_ptr<PatternMatchingQueryProxy> AtomSpace::pattern_matching_query(
    const std::vector<std::string>& query, unsigned int answers_count) {}

unsigned int AtomSpace::pattern_matching_count(const std::vector<std::string>& query) {}

void AtomSpace::pattern_matching_fetch(const std::vector<std::string>& query,
                                       unsigned int answers_count) {}

char* AtomSpace::add_node(const std::string& type, const std::string& name) {}

char* AtomSpace::add_link(const std::string& type, const std::vector<std::string>& targets) {}

void AtomSpace::commit_changes(Scope scope) {}

}  // namespace atomspace