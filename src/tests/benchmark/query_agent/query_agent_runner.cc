#include "query_agent_runner.h"

QueryAgentRunner::QueryAgentRunner(int tid, shared_ptr<AtomSpace> atom_space, int iterations)
    : Runner(tid, iterations), atom_space_(atom_space) {}
