#pragma once

#include "DedicatedThread.h"
#include "Link.h"
#include "LinkTemplate.h"
#include "Operator.h"
#include "ThreadSafeHeap.h"
#include "ThreadSafeQueue.h"
#include "map"
#include "mutex"
#include "set"

using namespace std;
using namespace atoms;
using namespace processor;

namespace query_element {

/**
 * This operator takes as input a single query element and two handles (SOURCE and TARGET) and
 * outputs QueryAnswers which represent paths between SOURCE and TARGET. Additional parameters
 * (link selector and tail/head reference) are used to determine which handle in the QueryAnswer
 * is supposed to be used and which link targets are supposed to be used as edge tail and head.
 *
 * Each QueryAnswer in the input is supposed to have exatcly 1 handle,
 * i.e. query_answer->handles.size() == 1. In addition to this, the handle is supposed to be the
 * handle of a ternary link such as
 *
 * LINK
 *     TARGET1
 *     TARGET2
 *     TARGET3
 *
 * TARGET1 is disregarded. TARGET2 and TARGET3 are used to connect the paths. Just to make it
 * easy to write an example, lets assume the input handles represent links like these:
 *
 * (Similarity H1 H2)
 *
 * Each QueryAnswer in the Chain Operator output will contain N handles, representing a path
 * with N links connecting SOURCE and TARGET. Suppose we have a QueryAnswer with N=4, the handles
 * in query_answer->handles will point to links like these:
 *
 * (Similarity SOURCE H1)
 * (Similarity H1 H2)
 * (Similarity H2 H3)
 * (Similarity H3 TARGET)
 *
 * Chained to form a path between SOURCE and TARGET. Note that the first link target is
 * disregarded so you may have something like:
 *
 * CHAIN SOURCE TARGET
 *     OR 2
 *         LinkTemplate 3
 *             Node Equivalence
 *             Variable v1
 *             Variable v2
 *         LinkTemplate 3
 *             Node Similarity
 *             Variable v1
 *             Variable v2
 *
 * This cold produce QueryAnswers paths chaining Similarity and Equivalence links in the same path.
 * E.g.:
 *
 * (Similarity SOURCE H1)
 * (Similarity H1 H2)
 * (Equivalence H2 H3)
 * (Similarity H3 H4)
 * (Equivalence H4 H5)
 * (Similarrity H5 TARGET)
 *
 * Also note that, because input QueryAnswer are supposed to have exatcly 1 handle, the following
 * query IS NOT valid:
 *
 * CHAIN SOURCE TARGET
 *     AND 2
 *         LinkTemplate 3
 *             Node Equivalence
 *             Variable v1
 *             Variable v2
 *         LinkTemplate 3
 *             Node Equivalence
 *             Variable v2
 *             Variable v3
 *
 * Optionally, ALLOW_INCOMPLETE_CHAIN_PATH can be set true to determine that the Chain Operator
 * should output incomplete paths as well as complete ones (the same prioritizartion by
 * STI applies to incomplete * paths). All reported incomplete paths will contain either the
 * SOURCE as its first element or the TARGET in its end. So, the QueryAnswers of a Chain operator
 * may produce paths like these:
 *
 * (Similarity SOURCE H1)
 * (Similarity H1 H2)
 *
 * (Similarity SOURCE H1)
 * (Similarity H1 H2)
 * (Similarity H2 H3)
 *
 * ...
 *
 * (Similarity H2 H1)
 * (Similarity H1 TARGET)
 *
 * (Similarity H3 H2)
 * (Similarity H2 H1)
 * (Similarity H1 TARGET)
 */
class Chain : public Operator<1>, public ThreadMethod {
   public:
    // --------------------------------------------------------------------------------------------
    // Inner types

    class Path {
       public:
        vector<pair<pair<string, string>, shared_ptr<QueryAnswer>>> edges;
        double path_sti;
        bool forward_flag;
        Path(const string& tail, const string& head, QueryAnswer* answer, bool forward_flag) {
            if (tail == head) {
                Utils::error("Invalid cyclic edge: " + tail + " -> " + head);
            }
            this->edges.push_back({{tail, head}, shared_ptr<QueryAnswer>(answer)});
            this->path_sti = answer->importance;
            this->forward_flag = forward_flag;
        }
        Path(shared_ptr<Link> link,  // Used in tests
             QueryAnswer* answer,
             bool forward_flag)
            : Path(link->targets[1], link->targets[2], answer, forward_flag) {}
        Path(const Path& other) {
            this->edges = other.edges;
            this->path_sti = other.path_sti;
            this->forward_flag = other.forward_flag;
        }
        Path(bool forward_flag) {
            this->path_sti = 0;
            this->forward_flag = forward_flag;
        }
        Path& operator=(const Path& other) {
            this->edges = other.edges;
            this->path_sti = other.path_sti;
            this->forward_flag = other.forward_flag;
            return *this;
        }
        inline bool empty() { return this->edges.size() == 0; }
        inline unsigned int size() { return this->edges.size(); }
        inline void clear() {
            this->edges.clear();
            this->path_sti = 0;
        }
        inline void concatenate(const Path& other) {
            if (this->forward_flag != other.forward_flag) {
                Utils::error("Invalid attempt to merge incompatible HeapElements");
            }
            this->edges.insert(this->edges.end(), other.edges.begin(), other.edges.end());
            this->path_sti = max(this->path_sti, other.path_sti);
        }
        inline string end_point() {
            if (this->forward_flag) {
                return this->edges.back().first.second;
            } else {
                return this->edges.back().first.first;
            }
        }
        inline string start_point() {
            if (this->forward_flag) {
                return this->edges.front().first.first;
            } else {
                return this->edges.front().first.second;
            }
        }
        inline bool contains(string handle) {
            for (auto pair : this->edges) {
                if ((pair.first.first == handle) || (pair.first.second == handle)) {
                    return true;
                }
            }
            return false;
        }
        inline bool allow_concatenation(Path& other) {
            if ((this->size() == 0) || (other.size() == 0)) {
                return true;
            } else if (this->end_point() != other.start_point()) {
                return false;
            }
            // unsigned int this_index = (this->forward_flag ? 1 : 2);
            // unsigned int other_index = (this->forward_flag ? 2 : 1);
            for (auto pair_other : other.edges) {
                for (auto pair_this : this->edges) {
                    if (this->forward_flag) {
                        if (pair_other.first.second == pair_this.first.first) {
                            return false;
                        }
                    } else {
                        if (pair_other.first.first == pair_this.first.second) {
                            return false;
                        }
                    }
                }
            }
            return true;
        }
        string to_string();
    };

    typedef ThreadSafeHeap<Path, double> HeapType;

    // --------------------------------------------------------------------------------------------
    // Static variables

    static string ORIGIN_VARIABLE_NAME;
    static string DESTINY_VARIABLE_NAME;

    // --------------------------------------------------------------------------------------------
    // Public methods

    /**
     * Constructor.
     */
    Chain(const array<shared_ptr<QueryElement>, 1>& clauses,
          shared_ptr<LinkTemplate> link_template,
          const string& source_handle,
          const string& target_handle,
          const QueryAnswerElement& link_selector,
          unsigned int tail_reference,
          unsigned int head_reference);

    /**
     * Constructor. Typically used in tests, defaulting the link selector to the first handle in
     * the query answer and assuming a link like (disregarded $v1 $v2) where chaining will be made
     * assuming $v1 -> $v2.
     */
    Chain(const array<shared_ptr<QueryElement>, 1>& clauses,
          const string& source_handle,
          const string& target_handle);

    /**
     * Destructor.
     */
    ~Chain();

    /**
     * Thread-safe access to the source_index map.
     */
    shared_ptr<HeapType> get_source_index(const string& key);

    /**
     * Thread-safe access to the target_index map.
     */
    shared_ptr<HeapType> get_target_index(const string& key);

    /**
     * Chain Operator thread.
     * Report a QueryAnswer to the next query element in the query tree.
     */
    void report_path(Path& path);

    /**
     * Chain Operator thread.
     * Called after antecedent query element notifies that input has been ended and after
     * all input has already been acknowledged and properly turned into elementary Paths.
     */
    void set_all_input_acknowledged(bool flag);

    /**
     * Chain Operator AND Path Finder threads.
     * Check if all input has already been aknowledged (see set_all_input_acknowledged()).
     */
    bool all_input_acknowledged();

    /**
     * Path Finder thread.
     * Called after all possible paths between source and targetr has been explored. Basically,
     * notifies that the Chain Operator has ended the job of trying to find paths
     * (complete or incomplete).
     */
    void set_all_paths_explored(bool flag);

    /**
     * Chain Operator AND Path Finder threads.
     * Check if the Chain Operator has ended the search for new paths
     * (see set_all_paths_explored()).
     */
    bool all_paths_explored();

    /**
     * Chain Operator AND Path Finder threads.
     * Empties the refeed_buffer, a buffer that stores paths which are supposed to get back to be
     * re-evaluated by Pathg Finder when new (so yet unseen) input is read in the Chain Operator.
     */
    void refeed_paths();

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
        bool conditional_refeed(Path& path,
                                shared_ptr<HeapType>& candidates_heap,
                                unsigned int count_cycles);
    };

    void initialize(const array<shared_ptr<QueryElement>, 1>& clauses);

    shared_ptr<LinkTemplate> input_link_template;
    string source_handle;
    string target_handle;
    QueryAnswerElement link_selector;
    unsigned int tail_reference;
    unsigned int head_reference;
    PathFinder* forward_path_finder;
    PathFinder* backward_path_finder;
    shared_ptr<DedicatedThread> operator_thread;
    shared_ptr<DedicatedThread> forward_thread;
    shared_ptr<DedicatedThread> backward_thread;
    ThreadSafeQueue<Path> refeeding_buffer;
    set<string> known_links;
    set<string> reported_answers;
    map<string, shared_ptr<HeapType>> source_index;
    map<string, shared_ptr<HeapType>> target_index;
    bool all_input_acknowledged_flag;
    bool all_paths_explored_flag;
    mutex source_index_mutex;
    mutex target_index_mutex;
    mutex all_input_acknowledged_mutex;
    mutex all_paths_explored_mutex;
};

}  // namespace query_element
