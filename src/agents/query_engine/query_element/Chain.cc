#include "Chain.h"
#include "ThreadSafeHeap.h"
#include "AtomDBSingleton.h"
#include "Hasher.h"

#include "Logger.h"

using namespace query_element;
using namespace atomdb;
using namespace commons;

string Chain::ORIGIN_VARIABLE_NAME = "origin";
string Chain::DESTINY_VARIABLE_NAME = "destiny";

// -------------------------------------------------------------------------------------------------
// Public methods

Chain::Chain(const array<shared_ptr<QueryElement>, 1>& clauses,
             const string& source_handle,
             const string& target_handle) : Operator<1>(clauses) , source_handle(source_handle), target_handle(target_handle) {
    initialize(clauses);
}

Chain::~Chain() {
    LOG_DEBUG("Chain::~Chain() BEGIN");
    graceful_shutdown();
    delete this->forward_path_finder;
    delete this->backward_path_finder;
    LOG_DEBUG("~Chain::Chain() END");
}

shared_ptr<Chain::HeapType> Chain::get_source_index(const string& key) {
    lock_guard<mutex> semaphore(this->source_index_mutex);
    auto it = this->source_index.find(key);
    if (it == this->source_index.end()) {
        return nullptr;
    } else {
        return it->second;
    }
}

shared_ptr<Chain::HeapType> Chain::get_target_index(const string& key) {
    lock_guard<mutex> semaphore(this->target_index_mutex);
    auto it = this->target_index.find(key);
    if (it == this->target_index.end()) {
        return nullptr;
    } else {
        return it->second;
    }
}

// --------------------------------------------------------------------------------------------
// QueryElement API

void Chain::setup_buffers() {
    LOG_DEBUG("Chain::setup_buffers() BEGIN");
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
    LOG_DEBUG("Chain::setup_buffers() END");
}

void Chain::graceful_shutdown() {
    LOG_DEBUG("Chain::graceful_shutdown() BEGIN");
    if (! this->forward_thread->is_finished()) {
        this->forward_thread->stop();
    }
    if (! this->backward_thread->is_finished()) {
        this->backward_thread->stop();
    }
    if (! this->operator_thread->is_finished()) {
        this->operator_thread->stop();
    }
    Operator<1>::graceful_shutdown();
    LOG_DEBUG("Chain::graceful_shutdown() END");
}

// --------------------------------------------------------------------------------------------
// ThreadMethod API

bool Chain::PathFinder::thread_one_step() {
#if LOG_LEVEL >= DEBUG_LEVEL
    lock_guard<mutex> semaphore(this->chain_operator->thread_debug_mutex);
#endif
    if (this->chain_operator->all_paths_explored()) {
        return false;
    }
    LOG_DEBUG("[PATH_FINDER] " << (this->forward_flag ? "FORWARD" : "BACKWARD") << " PathFinder STEP");
    string cursor = this->origin;
    Path visited(this->forward_flag);
    do {
        LOG_DEBUG("[PATH_FINDER] " << "Cursor: " + cursor);
        shared_ptr<HeapType> base_heap = this->forward_flag ?
                                         this->chain_operator->get_source_index(cursor):
                                         this->chain_operator->get_target_index(cursor);

        if (base_heap == nullptr || base_heap->empty()) {
            LOG_DEBUG("[PATH_FINDER] " << "Empty or non-existent base_heap");
            if (this->chain_operator->all_input_aknowledged()) {
                // double check is required to avoid race condition
                shared_ptr<HeapType> check_heap = this->forward_flag ?
                                                  this->chain_operator->get_source_index(cursor):
                                                  this->chain_operator->get_target_index(cursor);
                if (check_heap == nullptr || check_heap->empty()) {
                    this->chain_operator->set_all_paths_explored(true);
                    LOG_DEBUG("[PATH_FINDER] " << "All paths has been explored");
                }
            }
            break;
        }
        Path previous_path = base_heap->top_and_pop();
        LOG_DEBUG("[PATH_FINDER] " << "Popped: " + previous_path.to_string());
        if (previous_path.end_point() == this->destiny) {
            LOG_DEBUG("[PATH_FINDER] " << "Reporting complete path: " << previous_path.to_string());
            this->chain_operator->report_path(previous_path);
            visited.concatenate(previous_path);
            break;
        }
        LOG_DEBUG("[PATH_FINDER] " << "Searching candidate paths " << (this->forward_flag ? "FROM " : "TO ") << previous_path.end_point());
        shared_ptr<HeapType> candidates_heap = this->forward_flag ?
                                               this->chain_operator->get_source_index(previous_path.end_point()):
                                               this->chain_operator->get_target_index(previous_path.end_point());
        if (candidates_heap->empty()) {
            LOG_DEBUG("[PATH_FINDER] " << "Found no candidates.");
            if (this->chain_operator->all_input_aknowledged() && candidates_heap->empty()) { // double check is required to avoid race condition
                LOG_DEBUG("[PATH_FINDER] " << "All input is aknowlkedged. Discarding dead-end path: " << previous_path.to_string());
            } else {
                LOG_DEBUG("[PATH_FINDER] " << "Still aknowledging input. Pushing " << previous_path.to_string() << " back.");
                base_heap->push(previous_path, previous_path.path_sti);
            }
            break;
        }

        vector<Path> candidates;
        candidates_heap->snapshot(candidates);
        Path new_path(this->forward_flag);
        Path best_path(this->forward_flag);
        double best_candidate_sti = -1;
        string best_candidate = "";
        for (Path candidate : candidates) {
            LOG_DEBUG("[PATH_FINDER] " << "Candidate: " << candidate.to_string());
            new_path = previous_path;
            new_path.concatenate(candidate);
            if (true) {
            //if (! new_path.has_cycles()) {
                if (candidate.path_sti > best_candidate_sti) {
                    LOG_DEBUG("[PATH_FINDER] " << "Candidate is the best so far. Resulting path is: " << new_path.to_string());
                    best_candidate_sti = candidate.path_sti;
                    best_candidate = candidate.end_point();
                    best_path = new_path;
                }
                LOG_DEBUG("[PATH_FINDER] " << "Pushing new path: " << new_path.to_string());
                base_heap->push(new_path, new_path.path_sti);
            } else {
                LOG_DEBUG("[PATH_FINDER] " << "Disregarding candidate because it adds a cycle: " << new_path.to_string());
            }
        }
        cursor = best_candidate;
        if (best_candidate_sti > 0) {
            LOG_DEBUG("[PATH_FINDER] " << "Reporting incomplete path: " << best_path.to_string());
            this->chain_operator->report_path(best_path);
            visited.concatenate(best_path);
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

    if (all_paths_explored()) {
        if (! this->forward_thread->is_finished()) {
            LOG_DEBUG("[CHAIN OPERATOR] " << "All paths explored. Stopping path finders...");
            this->forward_thread->stop();
            this->backward_thread->stop();
            LOG_DEBUG("[CHAIN OPERATOR] " << "All paths explored. Sopping path finders. DONE");
            LOG_DEBUG("[CHAIN OPERATOR] " << "All paths explored. Notifying output buffer...");
            this->output_buffer->query_answers_finished();
            LOG_DEBUG("[CHAIN OPERATOR] " << "All paths explored. Notifying output buffer. DONE");
        }
        return false;
    }
    if (all_input_aknowledged()) {
        return false;
    }
#if LOG_LEVEL >= DEBUG_LEVEL
    {
        lock_guard<mutex> semaphore(this->thread_debug_mutex);
#endif
    if ((answer = dynamic_cast<QueryAnswer*>(this->input_buffer[0]->pop_query_answer())) != NULL) {
        LOG_DEBUG("[CHAIN OPERATOR] " << "New query answer: " << answer->to_string());
        for (string handle : answer->handles) {
            auto iterator = this->known_links.find(handle);
            if (iterator == this->known_links.end()) {
                this->known_links.insert(iterator, handle);
                shared_ptr<Link> link = dynamic_pointer_cast<Link>(AtomDBSingleton::get_instance()->get_atom(handle));
                if (link == nullptr) {
                    LOG_DEBUG("[CHAIN OPERATOR] " << "NULL link");
                } else {
                    LOG_DEBUG("[CHAIN OPERATOR] " << "Valid link");
                }
                LOG_DEBUG("[CHAIN OPERATOR] " << "New link: " << link->to_string());
                if (link->arity() == 3) {
                    {
                        lock_guard<mutex> semaphore(this->source_index_mutex);
                        for (unsigned int i = 1; i <= 2; i++) {
                            if (this->source_index.find(link->targets[i]) == this->source_index.end()) {
                                this->source_index[link->targets[i]] = make_shared<HeapType>();
                            }
                        }
                        this->source_index[link->targets[1]]->push(Path(link, answer->importance, true), answer->importance);
                    }
                    {
                        lock_guard<mutex> semaphore(this->target_index_mutex);
                        for (unsigned int i = 1; i <= 2; i++) {
                            if (this->target_index.find(link->targets[i]) == this->target_index.end()) {
                                this->target_index[link->targets[i]] = make_shared<HeapType>();
                            }
                        }
                        this->target_index[link->targets[2]]->push(Path(link, answer->importance, false), answer->importance);
                    }
                } else {
                    Utils::error("Invalid Link " + link->to_string() + " with arity " + std::to_string(link->arity()) + " in CHAIN operator.");
                    break;
                }
            } else {
                LOG_DEBUG("[CHAIN OPERATOR] " << "Discarding already inserted handle: " << handle);
            }
        }
        return true;
    } else {
        if (this->input_buffer[0]->is_query_answers_finished() && this->input_buffer[0]->is_query_answers_empty()) {
            LOG_DEBUG("[CHAIN OPERATOR] " << "All input has been aknowledged");
            this->set_all_input_aknowledged(true);
        }
        return false;
    }
#if LOG_LEVEL >= DEBUG_LEVEL
    }
#endif
}

void Chain::report_path(Path& path) {
    QueryAnswer* query_answer = new QueryAnswer(path.path_sti);
    if (path.forward_flag) {
        for (auto pair : path.links) {
            query_answer->add_handle(pair.first->handle());
        }
    } else {
        for (auto pair = path.links.rbegin(); pair != path.links.rend(); ++pair) {
            query_answer->add_handle(pair->first->handle());
        }
    }
    string answer_hash = Hasher::composite_handle(query_answer->handles);
    if (this->reported_answers.find(answer_hash) == this->reported_answers.end()) {
        this->reported_answers.insert(answer_hash);
        query_answer->assignment.assign(ORIGIN_VARIABLE_NAME, path.start_point());
        query_answer->assignment.assign(DESTINY_VARIABLE_NAME, path.end_point());
        LOG_INFO("Reporting path: " << path.to_string());
        this->output_buffer->add_query_answer(query_answer);
    } else {
        delete query_answer;
    }
}

void Chain::set_all_input_aknowledged(bool flag) {
    lock_guard<mutex> semaphore(this->all_input_aknowledged_mutex);
    this->all_input_aknowledged_flag = flag;
}

bool Chain::all_input_aknowledged() {
    lock_guard<mutex> semaphore(this->all_input_aknowledged_mutex);
    return this->all_input_aknowledged_flag;
}

void Chain::set_all_paths_explored(bool flag) {
    lock_guard<mutex> semaphore(this->all_paths_explored_mutex);
    this->all_paths_explored_flag = flag;
}

bool Chain::all_paths_explored() {
    lock_guard<mutex> semaphore(this->all_paths_explored_mutex);
    return this->all_paths_explored_flag;
}

// --------------------------------------------------------------------------------------------
// Private stuff

void Chain::initialize(const array<shared_ptr<QueryElement>, 1>& clauses) {
    this->id = "CHAIN";
    this->all_input_aknowledged_flag = false;
    this->all_paths_explored_flag = false;
    this->forward_path_finder = new PathFinder(this, true);
    this->backward_path_finder = new PathFinder(this, false);
}
