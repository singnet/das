#pragma once

#include "Operator.h"
#include "DedicatedThread.h"
#include "ThreadSafeHeap.h"
#include "Link.h"
#include "map"
#include "set"
#include "mutex"

using namespace std;
using namespace atoms;
using namespace processor;

namespace query_element {

/**
 *
 */
class Chain : public Operator<1>, public ThreadMethod {

private:

    class Path {
        public:
        vector<pair<shared_ptr<Link>, double>> links;
        double path_sti;
        bool forward_flag;
        Path(shared_ptr<Link> link, double sti, bool forward_flag) {
            this->links.push_back({link, sti});
            this->path_sti = sti;
            this->forward_flag = forward_flag;
        }
        Path(const Path& other) {
            this->links = other.links;
            this->path_sti = other.path_sti;
            this->forward_flag = other.forward_flag;
        }
        Path(bool forward_flag) {
            this->path_sti = 0;
            this->forward_flag = forward_flag;
        }
        Path& operator=(const Path& other) {
            this->links = other.links;
            this->path_sti = other.path_sti;
            this->forward_flag = other.forward_flag;
            return *this;
        }
        inline bool empty() { return this->links.size() == 0; }
        inline unsigned int size() { return this->links.size(); }
        inline void clear() {
            this->links.clear();
            this->path_sti = 0;
        }
        inline void concatenate(const Path& other) {
            if (this->forward_flag != other.forward_flag) {
                Utils::error("Invalid attempt to merge incompatible HeapElements");
            }
            this->links.insert(this->links.end(), other.links.begin(), other.links.end());
            this->path_sti = max(this->path_sti, other.path_sti);
        }
        inline string end_point() {
            if (this->forward_flag) {
                return this->links.back().first->targets[2];
            } else {
                return this->links.back().first->targets[1];
            }
        }
        inline string start_point() {
            if (this->forward_flag) {
                return this->links.front().first->targets[1];
            } else {
                return this->links.front().first->targets[2];
            }
        }
        inline bool contains(string handle) {
            for (auto pair : this->links) {
                if ((pair.first->targets[1] == handle) || (pair.first->targets[2] == handle)) {
                    return true;
                }
            }
            return false;
        }
        string to_string() {
            string answer = "";
            bool first = true;
            string last_handle = "";
            string check_handle = "";
            for (auto pair : this->links) {
                if (first) {
                    first = false;
                    last_handle = this->forward_flag ? pair.first->targets[1] : pair.first->targets[2];
                    answer = last_handle;
                }
                check_handle = this->forward_flag ? pair.first->targets[1] : pair.first->targets[2];
                if (check_handle != last_handle) {
                    LOG_ERROR("Invalid Path");
                }
                last_handle = this->forward_flag ? pair.first->targets[2] : pair.first->targets[1];
                answer += this->forward_flag ? " -> " : " <- ";
                answer += last_handle;
            }
            return answer;
        }
    };

    typedef ThreadSafeHeap<Path, double> HeapType;

public:

    // --------------------------------------------------------------------------------------------
    // Public methods

    /**
     * Constructor.
     */
    Chain(const array<shared_ptr<QueryElement>, 1>& clauses,
          const string& source_handle,
          const string& target_handle);

    /**
     * Destructor.
     */
    ~Chain();

    static string ORIGIN_VARIABLE_NAME;
    static string DESTINY_VARIABLE_NAME;

    shared_ptr<HeapType> get_source_index(const string& key);
    shared_ptr<HeapType> get_target_index(const string& key);
    void report_path(Path& path);
    void set_all_input_aknowledged(bool flag);
    bool all_input_aknowledged();
    void set_all_paths_explored(bool flag);
    bool all_paths_explored();


    // --------------------------------------------------------------------------------------------
    // QueryElement API

    virtual void setup_buffers();
    virtual void graceful_shutdown();

    // --------------------------------------------------------------------------------------------
    // ThreadMethod API

    virtual bool thread_one_step();

    mutex thread_debug_mutex;

private:

    class PathFinder : public ThreadMethod {
        public:
        Chain* chain_operator;
        bool forward_flag;
        string origin;
        string destiny;
        PathFinder(Chain* chain_operator, bool forward_flag) {
            this->chain_operator = chain_operator;
            this->forward_flag = forward_flag;
            if (forward_flag) {
                origin = chain_operator->source_handle;
                destiny = chain_operator->target_handle;
            } else {
                origin = chain_operator->target_handle;
                destiny = chain_operator->source_handle;
            }
        }
        ~PathFinder() {}
        bool thread_one_step();
    };

    void initialize(const array<shared_ptr<QueryElement>, 1>& clauses);

    string source_handle;
    string target_handle;
    PathFinder* forward_path_finder;
    PathFinder* backward_path_finder;
    shared_ptr<DedicatedThread> operator_thread;
    shared_ptr<DedicatedThread> forward_thread;
    shared_ptr<DedicatedThread> backward_thread;
    set<string> known_links;
    set<string> reported_answers;
    map<string, shared_ptr<HeapType>> source_index;
    map<string, shared_ptr<HeapType>> target_index;
    bool all_input_aknowledged_flag;
    bool all_paths_explored_flag;
    mutex source_index_mutex;
    mutex target_index_mutex;
    mutex all_input_aknowledged_mutex;
    mutex all_paths_explored_mutex;
};

} // namespace query_element
