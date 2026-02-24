#pragma once

#include "Operator.h"
#include "DedicatedThread.h"
#include "ThreadSafeHeap.h"
#include "Link.h"
#include "map"
#include "set"
#include "mutex"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace atoms;
using namespace processor;

namespace query_element {

/**
 *
 */
class Chain : public Operator<1>, public ThreadMethod {

private:

    class HeapElement {
        public:
        vector<pair<shared_ptr<Link>, double>> path;
        double path_sti;
        bool forward_flag;
        HeapElement(shared_ptr<Link> link, double sti, bool forward_flag) {
            this->path.push_back({link, sti});
            this->path_sti = sti;
            this->forward_flag = forward_flag;
        }
        HeapElement(const HeapElement& other) {
            this->path = other.path;
            this->path_sti = other.path_sti;
            this->forward_flag = other.forward_flag;
        }
        HeapElement(bool forward_flag) {
            this->path_sti = 0;
            this->forward_flag = forward_flag;
        }
        HeapElement& operator=(const HeapElement& other) {
            this->path = other.path;
            this->path_sti = other.path_sti;
            this->forward_flag = other.forward_flag;
            return *this;
        }
        inline bool empty() { return this->path.size() == 0; }
        inline unsigned int size() { return this->path.size(); }
        inline void clear() {
            this->path.clear();
            this->path_sti = 0;
        }
        inline void push(shared_ptr<Link> link, double sti) {
            path.push_back({link, sti});
            this->path_sti = max(this->path_sti, sti);
        }
        inline void push(const HeapElement& other) {
            if (this->forward_flag != other.forward_flag) {
                Utils::error("Invalid attempt to merge incompatible HeapElements");
            }
            this->path.insert(this->path.end(), other.path.begin(), other.path.end());
            this->path_sti = max(this->path_sti, other.path_sti);
        }
        inline string endpoint() {
            if (this->forward_flag) {
                return this->path.back().first->targets[2];
            } else {
                return this->path.back().first->targets[1];
            }
        }
        inline bool contains(string handle) {
            for (auto pair : this->path) {
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
            for (auto pair : this->path) {
                if (first) {
                    first = false;
                    last_handle = this->forward_flag ? pair.first->targets[1] : pair.first->targets[2];
                    answer = last_handle;
                }
                check_handle = this->forward_flag ? pair.first->targets[1] : pair.first->targets[2];
                if (check_handle != last_handle) {
                    LOG_ERROR("Invalid path in HeapElement");
                }
                last_handle = this->forward_flag ? pair.first->targets[2] : pair.first->targets[1];
                answer += this->forward_flag ? " -> " : " <- ";
                answer += last_handle;
            }
            return answer;
        }
    };

    typedef ThreadSafeHeap<HeapElement, double> HeapType;

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

    shared_ptr<HeapType> get_source_index(const string& key);
    shared_ptr<HeapType> get_target_index(const string& key);
    void report_path(const HeapElement& path);

    // --------------------------------------------------------------------------------------------
    // QueryElement API

    virtual void setup_buffers();
    virtual void graceful_shutdown();

    // --------------------------------------------------------------------------------------------
    // ThreadMethod API

    virtual bool thread_one_step();


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
    map<string, shared_ptr<HeapType>> source_index;
    map<string, shared_ptr<HeapType>> target_index;
    mutex source_index_mutex;
    mutex target_index_mutex;
};

} // namespace query_element
