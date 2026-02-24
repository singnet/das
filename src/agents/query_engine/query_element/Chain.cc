#include "Chain.h"
#include "ThreadSafeHeap.h"
#include "AtomDBSingleton.h"

using namespace query_element;
using namespace atomdb;

// -------------------------------------------------------------------------------------------------
// Public methods

Chain::Chain(const array<shared_ptr<QueryElement>, 1>& clauses,
             const string& source_handle,
             const string& target_handle) : Operator<1>(clauses) , source_handle(source_handle), target_handle(target_handle) {
    initialize(clauses);
}

Chain::~Chain() { 
    graceful_shutdown(); 
    delete this->forward_path_finder;
    delete this->backward_path_finder;
}

shared_ptr<Chain::HeapType> Chain::get_source_index(const string& key) {
    lock_guard<mutex> semaphore(this->source_index_mutex);
    return this->source_index[key];
}

shared_ptr<Chain::HeapType> Chain::get_target_index(const string& key) {
    lock_guard<mutex> semaphore(this->target_index_mutex);
    return this->target_index[key];
}

// --------------------------------------------------------------------------------------------
// QueryElement API

void Chain::setup_buffers() {
    Operator<1>::setup_buffers();
    this->operator_thread = make_shared<DedicatedThread>(this->id + ":main_thread", this);
    this->operator_thread->setup();
    this->operator_thread->start();
    this->forward_thread = make_shared<DedicatedThread>(this->id + ":forward_thread", this->forward_path_finder);
    this->forward_thread->setup();
    this->forward_thread->start();
    this->backward_thread = make_shared<DedicatedThread>(this->id + ":backward_thread", this->backward_path_finder);
    this->backward_thread->setup();
    this->backward_thread->start();
}

void Chain::graceful_shutdown() {
    Operator<1>::graceful_shutdown();
    this->operator_thread->stop();
    this->forward_thread->stop();
    this->backward_thread->stop();
}

// --------------------------------------------------------------------------------------------
// ThreadMethod API

bool Chain::PathFinder::thread_one_step() {
    LOG_DEBUG("[PATH_FINDER] " << (this->forward_flag ? "FORWARD" : "BACKWARD") << " PathFinder STEP");
    string cursor = this->origin;
    HeapElement visited(this->forward_flag);
    do {
        LOG_DEBUG("[PATH_FINDER] " << "Cursor: " + cursor);
        shared_ptr<HeapType> base_heap = this->forward_flag ?
                                         this->chain_operator->get_source_index(cursor):
                                         this->chain_operator->get_target_index(cursor);

        HeapElement previous_path = base_heap->top_and_pop();
        LOG_DEBUG("[PATH_FINDER] " << "Popped: " + previous_path.to_string());
        LOG_DEBUG("[PATH_FINDER] " << "Searching candidate paths " << (this->forward_flag ? "FROM " : "TO ") << previous_path.endpoint());
        shared_ptr<HeapType> candidates_heap = this->forward_flag ?
                                               this->chain_operator->get_source_index(previous_path.endpoint()):
                                               this->chain_operator->get_target_index(previous_path.endpoint());
        if (candidates_heap->empty()) {
            LOG_DEBUG("[PATH_FINDER] " << "No candidates found. Pushing " << previous_path.to_string() " back.");
            base_heap->push(previous_path, previous_path.path_sti);
            break;
        }

        vector<HeapElement> candidates;
        candidates_heap->snapshot(candidates);
        HeapElement new_path(this->forward_flag);
        HeapElement best_path(this->forward_flag);
        double best_candidate_sti = -1;
        string best_candidate = "";
        for (HeapElement candidate : candidates) {
            LOG_DEBUG("[PATH_FINDER] " << "Candidate: " << candidate.to_string());
            new_path = previous_path;
            new_path.push(candidate);
            if (candidate.path_sti > best_candidate_sti) {
                LOG_DEBUG("[PATH_FINDER] " << "Candidate is the best so far. Resulting path is: " << new_path.to_string());
                best_candidate_sti = candidate.path_sti;
                best_candidate = candidate.endpoint();
                best_path = new_path;
            }
            LOG_DEBUG("[PATH_FINDER] " << "Pushing new path: " << new_path.to_string());
            base_heap->push(new_path, new_path.path_sti);
        }
        cursor = best_candidate;
        if (best_candidate_sti > 0) {
            LOG_DEBUG("[PATH_FINDER] " << "Reporting best new path: " << best_path.to_string());
            this->chain_operator->report_path(best_path);
            visited.push(best_path);
            best_path.clear();
        }
        LOG_DEBUG("[PATH_FINDER] " << "Iteration ended. Destiny: " << this->destiny << ". Cursor: " << cursor << ". Visited: " << visited.to_string());
    } while ((cursor != "") &&
             (! visited.contains(cursor)) &&
             (cursor != this->destiny));

    LOG_DEBUG("[PATH_FINDER] " << (this->forward_flag ? "FORWARD" : "BACKWARD") << " PathFinder STEP finished. " << (visited.size() > 0 ? "" : "Nothing changed. Going to sleep."));
    return (! visited.empty());
}

bool Chain::thread_one_step() {
    QueryAnswer* answer;
    if ((answer = dynamic_cast<QueryAnswer*>(this->input_buffer[0]->pop_query_answer())) != NULL) {
        for (string handle : answer->handles) {
            auto iterator = this->known_links.find(handle);
            if (iterator == this->known_links.end()) {
                this->known_links.insert(iterator, handle);
                shared_ptr<Link> link = dynamic_pointer_cast<Link>(AtomDBSingleton::get_instance()->get_atom(handle));
                LOG_DEBUG("[CHAIN OPERATOR] " << "New link: " << link->to_string());
                if (link->arity() == 3) {
                    {
                        lock_guard<mutex> semaphore(this->source_index_mutex);
                        for (unsigned int i = 1; i <= 2; i++) {
                            if (this->source_index.find(link->targets[i]) == this->source_index.end()) {
                                this->source_index[link->targets[i]] = make_shared<HeapType>();
                            }
                        }
                        this->source_index[link->targets[1]]->push(HeapElement(link, answer->importance, true), answer->importance);
                    }
                    {
                        lock_guard<mutex> semaphore(this->target_index_mutex);
                        for (unsigned int i = 1; i <= 2; i++) {
                            if (this->target_index.find(link->targets[i]) == this->target_index.end()) {
                                this->target_index[link->targets[i]] = make_shared<HeapType>();
                            }
                        }
                        this->target_index[link->targets[2]]->push(HeapElement(link, answer->importance, false), answer->importance);
                    }
                } else {
                    Utils::error("Invalid Link " + link->to_string() + " with arity " + std::to_string(link->arity()) + " in CHAIN operator.");
                    break;
                }
            } else {
                LOG_DEBUG("[CHAIN OPERATOR] " << "Discarding already inserted link: " << link->to_string());
            }
        }
        return true;
    } else {
        return false;
    }
}

void Chain::report_path(const HeapElement& path) {
    LOG_DEBUG("[PATH_FINDER] " << "Chain::report_path() " << path.to_string());
}

// --------------------------------------------------------------------------------------------
// Private stuff

void Chain::initialize(const array<shared_ptr<QueryElement>, 1>& clauses) {
    this->id = "CHAIN";
    this->forward_path_finder = new PathFinder(this, true);
    this->backward_path_finder = new PathFinder(this, false);
}
