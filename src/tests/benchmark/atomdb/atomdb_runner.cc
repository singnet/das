#include "atomdb_runner.h"

AtomDBRunner::AtomDBRunner(int tid, shared_ptr<AtomDB> db, int iterations)
    : Runner(tid, iterations), db_(db) {}

vector<string> AtomDBRunner::get_random_link_handle(shared_ptr<atomdb_api_types::HandleSet> handle_set,
                                                    size_t max_count,
                                                    size_t n) {
    vector<char*> link_handles;

    auto it = handle_set->get_iterator();

    char* handle = nullptr;
    while ((handle = it->next()) != nullptr && (link_handles.size() < max_count)) {
        link_handles.push_back(handle);
    }

    if (link_handles.empty()) {
        return {};
    }

    auto handles_to_return = min(n, link_handles.size());

    vector<string> handles;
    for (size_t i = 0; i < handles_to_return; ++i) {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<size_t> dist(0, link_handles.size() - 1);
        auto h = link_handles[dist(gen)];
        handles.push_back(string(h));
    }
    return handles;
};