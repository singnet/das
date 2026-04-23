#include "Logger.h"
#include "Chain.h"

#include "AtomDBSingleton.h"
#include "Hasher.h"
#include "ThreadSafeHeap.h"

using namespace query_element;
using namespace atomdb;
using namespace commons;

string Chain::ORIGIN_VARIABLE_NAME = "origin";
string Chain::DESTINY_VARIABLE_NAME = "destiny";

static string convert_handle(const string& handle) {
#if LOG_LEVEL >= DEBUG_LEVEL
    shared_ptr<Node> node =
        dynamic_pointer_cast<Node>(AtomDBSingleton::get_instance()->get_atom(handle));
    if (node != nullptr) {
        return node->name;
    }
#endif
    return handle;
}

// -------------------------------------------------------------------------------------------------
// Public methods

Chain::Chain(const array<shared_ptr<QueryElement>, 1>& clauses,
             shared_ptr<LinkTemplate> link_template,
             const string& source_handle,
             const string& target_handle,
             SearchDirection search_direction,
             const QueryAnswerElement& link_selector,
             unsigned int tail_reference,
             unsigned int head_reference,
             bool allow_incomplete_chain_path)
    : Operator<1>(clauses),
      input_link_template(link_template),
      source_handle(source_handle),
      target_handle(target_handle),
      search_direction(search_direction),
      link_selector(link_selector),
      tail_reference(tail_reference),
      head_reference(head_reference),
      allow_incomplete_chain_path(allow_incomplete_chain_path) {
    initialize(clauses);
}

Chain::Chain(const array<shared_ptr<QueryElement>, 1>& clauses,
             const string& source_handle,
             const string& target_handle,
             bool allow_incomplete_chain_path)
    : Chain(clauses,
            nullptr,
            source_handle,
            target_handle,
            BOTH,
            QueryAnswerElement(0),
            1,
            2,
            allow_incomplete_chain_path) {}

Chain::~Chain() {
    LOG_DEBUG("Chain::~Chain() BEGIN");
    graceful_shutdown();
    if (this->forward_path_finder != NULL) {
        delete this->forward_path_finder;
    }
    if (this->backward_path_finder != NULL) {
        delete this->backward_path_finder;
    }
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
    if (this->forward_path_finder != NULL) {
        this->forward_thread =
            make_shared<DedicatedThread>(this->id + ":forward_thread", this->forward_path_finder);
        this->forward_thread->setup();
        this->forward_thread->start();
    } else {
        this->forward_thread = nullptr;
    }
    if (this->backward_path_finder != NULL) {
        this->backward_thread =
            make_shared<DedicatedThread>(this->id + ":backward_thread", this->backward_path_finder);
        this->backward_thread->setup();
        this->backward_thread->start();
    } else {
        this->backward_thread = nullptr;
    }
    LOG_DEBUG("Chain::setup_buffers() END");
}

void Chain::graceful_shutdown() {
    LOG_DEBUG("Chain::graceful_shutdown() BEGIN");
    if ((this->forward_thread != nullptr) && !this->forward_thread->is_finished()) {
        this->forward_thread->stop();
    }
    if ((this->backward_thread != nullptr) && !this->backward_thread->is_finished()) {
        this->backward_thread->stop();
    }
    if (!this->operator_thread->is_finished()) {
        this->operator_thread->stop();
    }
    Operator<1>::graceful_shutdown();
    LOG_DEBUG("Chain::graceful_shutdown() END");
}

// --------------------------------------------------------------------------------------------
// ThreadMethod API

bool Chain::PathFinder::conditional_refeed(Path& path,
                                           shared_ptr<HeapType>& candidates_heap,
                                           unsigned int count_cycles) {
    if (this->chain_operator->all_input_acknowledged() &&
        (candidates_heap->empty() || (count_cycles == candidates_heap->size()))) {
        LOG_DEBUG("[PATH_FINDER] "
                  << "All input is acknowledged. Discarding dead-end path: " << path.to_string());
        return false;
    } else {
        LOG_DEBUG("[PATH_FINDER] "
                  << "Still acknowledging input. Pushing " << path.to_string()
                  << " back to refeeding buffer.");
        if (this->forward_flag) {
            this->chain_operator->refeeding_buffer_forward.push(path);
        } else {
            this->chain_operator->refeeding_buffer_backward.push(path);
        }
        LOG_DEBUG("[PATH_FINDER] Pushed. ");
        return true;
    }
}

void Chain::PathFinder::refeed_paths() {
    if (this->forward_flag) {
        this->chain_operator->refeed_paths_forward();
    } else {
        this->chain_operator->refeed_paths_backward();
    }
}

bool Chain::PathFinder::thread_one_step() {
#if LOG_LEVEL >= DEBUG_LEVEL
    lock_guard<mutex> semaphore(this->chain_operator->thread_debug_mutex);
#endif
    if (this->chain_operator->all_paths_explored()) {
        return false;
    }
    LOG_DEBUG("[PATH_FINDER] " << (this->forward_flag ? "FORWARD" : "BACKWARD") << " PathFinder STEP");
    shared_ptr<HeapType> base_heap = this->forward_flag
                                         ? this->chain_operator->get_source_index(this->origin)
                                         : this->chain_operator->get_target_index(this->origin);

    if (base_heap->empty()) {
        LOG_DEBUG("[PATH_FINDER] "
                  << "Empty base_heap. Trying to refeed paths.");
        this->refeed_paths();
        if (base_heap->empty()) {
            LOG_DEBUG("[PATH_FINDER] "
                      << "No paths to refeed.");
            if (this->chain_operator->all_input_acknowledged()) {
                // double check is required to avoid race condition
                shared_ptr<HeapType> check_heap =
                    this->forward_flag ? this->chain_operator->get_source_index(this->origin)
                                       : this->chain_operator->get_target_index(this->origin);
                if (check_heap == nullptr || check_heap->empty()) {
                    this->chain_operator->set_all_paths_explored(true);
                    LOG_DEBUG("[PATH_FINDER] "
                              << "All paths has been explored");
                }
            }
            return false;
        } else {
            LOG_DEBUG("[PATH_FINDER] "
                      << "Paths has been refed.");
        }
    }

    Path previous_path = base_heap->top_and_pop();
    LOG_DEBUG("[PATH_FINDER] "
              << "Popped: " << previous_path.to_string());
    if (previous_path.end_point() == this->destiny) {
        LOG_DEBUG("[PATH_FINDER] "
                  << "Found complete path: " << previous_path.to_string());
        this->chain_operator->report_path(previous_path);
        return true;
    }

    LOG_DEBUG("[PATH_FINDER] "
              << "Searching candidate paths " << (this->forward_flag ? "FROM " : "TO ")
              << convert_handle(previous_path.end_point()));
    shared_ptr<HeapType> candidates_heap =
        this->forward_flag ? this->chain_operator->get_source_index(previous_path.end_point())
                           : this->chain_operator->get_target_index(previous_path.end_point());
    if (candidates_heap->empty()) {
        LOG_DEBUG("[PATH_FINDER] "
                  << "Found no candidates.");
        return !conditional_refeed(previous_path, candidates_heap, 0);
    } else {
        LOG_DEBUG("[PATH_FINDER] "
                  << "Found " << candidates_heap->size());
    }

    vector<Path> candidates;
    candidates_heap->snapshot(candidates);
    Path new_path(this->forward_flag);
    Path best_path(this->forward_flag);
    double best_sti = -1;
    unsigned int count_cycles = 0;
    for (Path candidate : candidates) {
        LOG_DEBUG("[PATH_FINDER] "
                  << "Candidate: " << candidate.to_string());
        if (previous_path.allow_concatenation(candidate)) {
            new_path = previous_path;
            new_path.concatenate(candidate);
            if (candidate.path_sti > best_sti) {
                LOG_DEBUG("[PATH_FINDER] "
                          << "Candidate is the best so far. Resulting path is: "
                          << new_path.to_string());
                best_sti = candidate.path_sti;
                best_path = new_path;
            }
            LOG_DEBUG("[PATH_FINDER] "
                      << "Pushing new path: " << new_path.to_string());
            base_heap->push(new_path, new_path.path_sti);
        } else {
            LOG_DEBUG("[PATH_FINDER] Discarding because candidate would lead to a cycle.");
            count_cycles++;
        }
    }
    if (best_sti >= 0) {
        LOG_DEBUG("[PATH_FINDER] "
                  << "Best path: " << best_path.to_string());
        this->chain_operator->report_path(best_path);
        return true;
    } else {
        LOG_DEBUG("[PATH_FINDER] "
                  << "No suitable candidate.");
        return !conditional_refeed(previous_path, candidates_heap, count_cycles);
    }
}

void Chain::refeed_paths_forward() {
    lock_guard<mutex> semaphore(this->refeed_paths_forward_mutex);
    while (!this->refeeding_buffer_forward.empty()) {
        Path path = refeeding_buffer_forward.front_and_pop();
        LOG_DEBUG("Refeeding: " << path.to_string());
        this->source_index_mutex.lock();
        this->source_index[this->source_handle]->push(path, path.path_sti);
        this->source_index_mutex.unlock();
    }
}

void Chain::refeed_paths_backward() {
    lock_guard<mutex> semaphore(this->refeed_paths_backward_mutex);
    while (!this->refeeding_buffer_backward.empty()) {
        Path path = refeeding_buffer_backward.front_and_pop();
        LOG_DEBUG("Refeeding: " << path.to_string());
        this->target_index_mutex.lock();
        this->target_index[this->target_handle]->push(path, path.path_sti);
        this->target_index_mutex.unlock();
    }
}

bool Chain::thread_one_step() {
    QueryAnswer* answer;

    if (all_paths_explored()) {
        if (!this->path_finders_stopped) {
            this->path_finders_stopped = true;
            LOG_DEBUG("[CHAIN OPERATOR] " << "All paths explored. Stopping path finders...");
            if (forward_thread != nullptr) {
                this->forward_thread->stop();
            }
            if (backward_thread != nullptr) {
                this->backward_thread->stop();
            }
            LOG_DEBUG("[CHAIN OPERATOR] " << "All paths explored. Stopping path finders. DONE");
            LOG_DEBUG("[CHAIN OPERATOR] " << "All paths explored. Notifying output buffer...");
            this->output_buffer->query_answers_finished();
            LOG_DEBUG("[CHAIN OPERATOR] " << "All paths explored. Notifying output buffer. DONE");
        }
        return false;
    }
    if (all_input_acknowledged()) {
        return false;
    }
#if LOG_LEVEL >= DEBUG_LEVEL
    {
        lock_guard<mutex> semaphore(this->thread_debug_mutex);
#endif
        if ((answer = dynamic_cast<QueryAnswer*>(this->input_buffer[0]->pop_query_answer())) != NULL) {
            LOG_DEBUG("[CHAIN OPERATOR] "
                      << "New query answer: " << answer->to_string());
            string handle = answer->get(this->link_selector);
            auto iterator = this->known_links.find(handle);
            if (iterator == this->known_links.end()) {
                this->known_links.insert(iterator, handle);
                shared_ptr<Link> link =
                    dynamic_pointer_cast<Link>(AtomDBSingleton::get_instance()->get_atom(handle));
                if (link == nullptr) {
                    Utils::error("Invalid query answer in Chain operator.");
                } else {
                    LOG_DEBUG("[CHAIN OPERATOR] "
                              << "Valid link");
                }
                LOG_DEBUG("[CHAIN OPERATOR] "
                          << "New link: " << link->to_string());
                if (link->arity() > max(this->tail_reference, this->head_reference)) {
                    string tail = link->targets[this->tail_reference];
                    string head = link->targets[this->head_reference];
                    if (forward_active()) {
                        lock_guard<mutex> semaphore(this->source_index_mutex);
                        for (string key : {tail, head}) {
                            if (this->source_index.find(key) == this->source_index.end()) {
                                this->source_index[key] = make_shared<HeapType>();
                            }
                        }
                        this->source_index[tail]->push(Path(tail, head, answer, true),
                                                       answer->importance);
                    }
                    if (backward_active()) {
                        lock_guard<mutex> semaphore(this->target_index_mutex);
                        for (string key : {tail, head}) {
                            if (this->target_index.find(key) == this->target_index.end()) {
                                this->target_index[key] = make_shared<HeapType>();
                            }
                        }
                        this->target_index[head]->push(
                            Path(tail, head, QueryAnswer::copy(answer), false), answer->importance);
                    }
                } else {
                    Utils::error("Invalid Link " + link->to_string() + " with arity " +
                                 std::to_string(link->arity()) + " in CHAIN operator. Tail reference: " +
                                 std::to_string(this->tail_reference) +
                                 ". Head reference: " + std::to_string(this->head_reference));
                }
            } else {
                LOG_DEBUG("[CHAIN OPERATOR] "
                          << "Discarding already inserted handle: " << convert_handle(handle));
            }
            LOG_DEBUG("[CHAIN OPERATOR] "
                      << "Refeeding paths");
            if (forward_active()) {
                refeed_paths_forward();
            }
            if (backward_active()) {
                refeed_paths_backward();
            }
            LOG_DEBUG("[CHAIN OPERATOR] "
                      << "Done refeeding");
            return true;
        } else {
            if (this->input_buffer[0]->is_query_answers_finished() &&
                this->input_buffer[0]->is_query_answers_empty()) {
                LOG_DEBUG("[CHAIN OPERATOR] "
                          << "All input has been acknowledged");
                this->set_all_input_acknowledged(true);
            }
            return false;
        }
#if LOG_LEVEL >= DEBUG_LEVEL
    }
#endif
}

void Chain::report_path(Path& path) {
    lock_guard<mutex> semaphore(this->reported_answers_mutex);
    bool complete_flag =
        ((path.start_point() == this->source_handle) && (path.end_point() == this->target_handle)) ||
        ((path.start_point() == this->target_handle) && (path.end_point() == this->source_handle));

    if (complete_flag || this->allow_incomplete_chain_path) {
        QueryAnswer* query_answer = new QueryAnswer(path.path_sti);
        query_answer->strength = 1;
        unsigned int path_index = query_answer->add_path();
        if (path.forward_flag) {
            for (auto pair : path.edges) {
                query_answer->add_path_element(path_index, pair.second->get(this->link_selector));
            }
        } else {
            for (auto pair = path.edges.rbegin(); pair != path.edges.rend(); ++pair) {
                query_answer->add_path_element(path_index, pair->second->get(this->link_selector));
            }
        }
        string answer_hash = Hasher::composite_handle(query_answer->get_path_vector(path_index));
        if (this->reported_answers.find(answer_hash) == this->reported_answers.end()) {
            this->reported_answers.insert(answer_hash);
            query_answer->assignment.assign(ORIGIN_VARIABLE_NAME, path.start_point());
            query_answer->assignment.assign(DESTINY_VARIABLE_NAME, path.end_point());
            string tag = (complete_flag ? "complete" : "incomplete");
            LOG_INFO("Reporting " << tag << " path: " << path.to_string());
            this->output_buffer->add_query_answer(query_answer);
        } else {
            delete query_answer;
        }
    } else {
        LOG_INFO("Incomplete path not reported: " << path.to_string());
    }
}

void Chain::set_all_input_acknowledged(bool flag) {
    lock_guard<mutex> semaphore(this->all_input_acknowledged_mutex);
    this->all_input_acknowledged_flag = flag;
}

bool Chain::all_input_acknowledged() {
    lock_guard<mutex> semaphore(this->all_input_acknowledged_mutex);
    return this->all_input_acknowledged_flag;
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
    if (clauses.size() != 1) {
        Utils::error("Invalid Chain operator with " + std::to_string(clauses.size()) + " clauses.");
    }
    this->id = "CHAIN(" + clauses[0]->id + ", " + this->source_handle + ", " + this->target_handle + ")";
    this->all_input_acknowledged_flag = false;
    this->all_paths_explored_flag = false;
    if (forward_active()) {
        this->forward_path_finder = new PathFinder(this, true);
    } else {
        this->forward_path_finder = NULL;
    }
    if (backward_active()) {
        this->backward_path_finder = new PathFinder(this, false);
    } else {
        this->backward_path_finder = NULL;
    }
    this->path_finders_stopped = false;
    this->source_index[this->source_handle] = make_shared<HeapType>();
    this->source_index[this->target_handle] = make_shared<HeapType>();
    this->target_index[this->source_handle] = make_shared<HeapType>();
    this->target_index[this->target_handle] = make_shared<HeapType>();
}

string Chain::Path::to_string() {
    string answer = "";
    bool first = true;
    string last_handle = "";
    string check_handle = "";
    for (auto pair : this->edges) {
        if (first) {
            first = false;
            last_handle = convert_handle(this->forward_flag ? pair.first.first : pair.first.second);
            answer = last_handle;
        }
        check_handle = convert_handle(this->forward_flag ? pair.first.first : pair.first.second);
        if (check_handle != last_handle) {
            LOG_ERROR("Invalid Path");
        }
        last_handle = convert_handle(this->forward_flag ? pair.first.second : pair.first.first);
        answer += this->forward_flag ? " -> " : " <- ";
        answer += last_handle;
    }
    return answer;
}
