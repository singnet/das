#include "QueryEvolutionProcessor.h"

#include "AttentionBrokerClient.h"
#include "LinkSchema.h"
#include "QueryEvolutionProxy.h"
#include "ServiceBus.h"
#include "Hasher.h"
#include "ServiceBusSingleton.h"

#define LOG_LEVEL DEBUG_LEVEL
#include "Logger.h"

using namespace evolution;
using namespace query_engine;
using namespace atoms;
using namespace service_bus;
using namespace attention_broker;

// -------------------------------------------------------------------------------------------------
// Constructors and destructors

QueryEvolutionProcessor::QueryEvolutionProcessor()
    : BusCommandProcessor({ServiceBus::QUERY_EVOLUTION}) {}

QueryEvolutionProcessor::~QueryEvolutionProcessor() {}

// -------------------------------------------------------------------------------------------------
// Public methods

shared_ptr<BusCommandProxy> QueryEvolutionProcessor::factory_empty_proxy() {
    shared_ptr<QueryEvolutionProxy> proxy(new QueryEvolutionProxy());
    return proxy;
}

void QueryEvolutionProcessor::run_command(shared_ptr<BusCommandProxy> proxy) {
    lock_guard<mutex> semaphore(this->query_threads_mutex);
    auto query_proxy = dynamic_pointer_cast<QueryEvolutionProxy>(proxy);
    string thread_id = "thread<" + proxy->my_id() + "_" + std::to_string(proxy->get_serial()) + ">";
    LOG_DEBUG("Starting new thread: " << thread_id << " to run command: <" << proxy->get_command()
                                      << ">");
    if (this->query_threads.find(thread_id) != this->query_threads.end()) {
        Utils::error("Invalid thread id: " + thread_id);
    } else {
        shared_ptr<StoppableThread> stoppable_thread = make_shared<StoppableThread>(thread_id);
        stoppable_thread->attach(new thread(
            &QueryEvolutionProcessor::thread_process_one_query, this, stoppable_thread, query_proxy));
        this->query_threads[thread_id] = stoppable_thread;
    }
}

// -------------------------------------------------------------------------------------------------
// Private methods

void QueryEvolutionProcessor::thread_process_one_query(shared_ptr<StoppableThread> monitor,
                                                       shared_ptr<QueryEvolutionProxy> proxy) {
    try {
        if (proxy->args.size() < 2) {
            Utils::error("Syntax error in query command. Missing implicit parameters.");
        }
        proxy->untokenize(proxy->args);
        string command = proxy->get_command();
        if (command == ServiceBus::QUERY_EVOLUTION) {
            LOG_DEBUG("QUERY_EVOLUTION proxy: " << proxy->to_string());
            this->evolve_query(monitor, proxy);
        } else {
            Utils::error("Invalid command " + command + " in QueryEvolutionProcessor");
        }
    } catch (const std::runtime_error& exception) {
        proxy->raise_error_on_peer(exception.what());
    } catch (const std::exception& exception) {
        proxy->raise_error_on_peer(exception.what());
    }
    LOG_DEBUG("Command finished: <" << proxy->get_command() << ">");
    // TODO add a call to remove_query_thread(monitor->get_id());
}

shared_ptr<PatternMatchingQueryProxy> QueryEvolutionProcessor::issue_sampling_query(
    shared_ptr<QueryEvolutionProxy> proxy) {
    auto pm_proxy =
        make_shared<PatternMatchingQueryProxy>(proxy->get_query_tokens(), proxy->get_context());
    pm_proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = true;
    pm_proxy->parameters[BaseQueryProxy::ATTENTION_UPDATE_FLAG] =
        false;  // (this->generation_count == 1);
    pm_proxy->parameters[BaseQueryProxy::USE_LINK_TEMPLATE_CACHE] = false;
    pm_proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = false;
    pm_proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] =
        proxy->parameters.get<bool>(BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS);
    pm_proxy->parameters[PatternMatchingQueryProxy::MAX_ANSWERS] =
        proxy->parameters.get<unsigned int>(QueryEvolutionProxy::POPULATION_SIZE);
    ServiceBusSingleton::get_instance()->issue_bus_command(pm_proxy);
    return pm_proxy;
}

shared_ptr<PatternMatchingQueryProxy> QueryEvolutionProcessor::issue_correlation_query(
    shared_ptr<QueryEvolutionProxy> proxy, vector<string> query_tokens) {
    auto pm_proxy = make_shared<PatternMatchingQueryProxy>(query_tokens, proxy->get_context());
    pm_proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = true;
    pm_proxy->parameters[BaseQueryProxy::ATTENTION_UPDATE_FLAG] = false;
    pm_proxy->parameters[BaseQueryProxy::USE_LINK_TEMPLATE_CACHE] = false;
    pm_proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] =
        (this->generation_count > 1);
    pm_proxy->parameters[BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS] =
        proxy->parameters.get<bool>(BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS);
    pm_proxy->parameters[PatternMatchingQueryProxy::MAX_ANSWERS] =
        proxy->parameters.get<unsigned int>(QueryEvolutionProxy::POPULATION_SIZE) * 5;

    ServiceBusSingleton::get_instance()->issue_bus_command(pm_proxy);
    return pm_proxy;
}

void QueryEvolutionProcessor::sample_population(
    shared_ptr<StoppableThread> monitor,
    shared_ptr<QueryEvolutionProxy> proxy,
    vector<std::pair<shared_ptr<QueryAnswer>, float>>& population) {
    bool remote_fitness = proxy->is_fitness_function_remote();
    vector<string> answer_bundle_vector;
    unsigned int population_size =
        proxy->parameters.get<unsigned int>(QueryEvolutionProxy::POPULATION_SIZE);

    auto pm_query = issue_sampling_query(proxy);

    unsigned int positive_importance_count = 0;
    unsigned int visited_count = this->visited_individuals.size();
    while ((!pm_query->finished()) && (!monitor->stopped()) && (population.size() < population_size)) {
        shared_ptr<QueryAnswer> answer = pm_query->pop();
        if (answer != NULL) {
            float fitness;
            if (remote_fitness) {
                fitness = 0;
                proxy->populate_metta_mapping(answer.get());
                answer_bundle_vector.push_back(answer->tokenize());
            } else {
                fitness = (remote_fitness ? 0 : proxy->compute_fitness(answer));
            }
            answer->strength = fitness;
            if (answer->importance > 0) {
                positive_importance_count++;
            }
            LOG_DEBUG("Sampling: " + answer->to_string());
            population.push_back(make_pair(answer, fitness));
            this->visited_individuals.insert(Hasher::composite_handle(answer->handles));
        } else {
            Utils::sleep();
        }
    }
    double renew_rate = ((double) this->visited_individuals.size() - visited_count) / population_size;
    LOG_INFO("Generation: " + std::to_string(this->generation_count) +
             " - Individuals with non-zero importance: " + std::to_string(positive_importance_count));
    LOG_INFO("Generation: " + std::to_string(this->generation_count) +
             " - Renew rate: " + std::to_string(renew_rate));
    if (!pm_query->finished()) {
        pm_query->abort();
        unsigned int count = 0;
        while (!pm_query->finished()) {
            shared_ptr<QueryAnswer> query_answer;
            if ((query_answer = pm_query->pop()) == NULL) {
                Utils::sleep();
            } else {
                count++;
            }
        }
        if (count > 0) {
            LOG_DEBUG("Discarding " << count << " answers");
        }
    }
    if (remote_fitness && (answer_bundle_vector.size() > 0)) {
        LOG_INFO("Evaluating fitness remotelly");
        if (answer_bundle_vector.size() >
            proxy->parameters.get<unsigned int>(BaseQueryProxy::MAX_BUNDLE_SIZE)) {
            Utils::error(
                "Expected POPULATION_SIZE <= MAX_BUNDLE_SIZE in order to use remote fitness evaluation");
            return;
        }
        proxy->remote_fitness_evaluation(answer_bundle_vector);
        while (!proxy->remote_fitness_evaluation_finished()) {
            Utils::sleep();
        }
        vector<float> fitness_bundle = proxy->get_remotely_evaluated_fitness();
        if (fitness_bundle.size() != population_size) {
            Utils::error("Invalid fitness bundle of size: " + std::to_string(fitness_bundle.size()));
            return;
        }
        for (unsigned int i = 0; i < population_size; i++) {
            float fitness = fitness_bundle[i];
            population[i].first->strength = fitness;
            population[i].second = fitness;
        }
    }
    // Sort decreasing by fitness value
    std::sort(population.begin(),
              population.end(),
              [](const std::pair<shared_ptr<QueryAnswer>, float>& left,
                 const std::pair<shared_ptr<QueryAnswer>, float>& right) {
                  return left.second > right.second;
              });
}

void QueryEvolutionProcessor::apply_elitism(
    shared_ptr<QueryEvolutionProxy> proxy,
    vector<std::pair<shared_ptr<QueryAnswer>, float>>& population,
    vector<std::pair<shared_ptr<QueryAnswer>, float>>& selected) {
    double elitism_rate = proxy->parameters.get<double>(QueryEvolutionProxy::ELITISM_RATE);
    unsigned int count = (unsigned int) std::lround(elitism_rate * population.size());
    if (count > 0) {
        if (count > population.size()) {
            Utils::error("Invalid evolution parameters. Elitism count: " + std::to_string(count) +
                         " population size: " + std::to_string(population.size()));
        } else {
            LOG_DEBUG("Selecting " << count << " individuals by elitism.");
            selected.insert(selected.begin(), population.begin(), population.begin() + count);
            population.erase(population.begin(), population.begin() + count);
        }
    }
}

void QueryEvolutionProcessor::select_one_by_tournament(
    shared_ptr<QueryEvolutionProxy> proxy,
    vector<std::pair<shared_ptr<QueryAnswer>, float>>& population,
    vector<std::pair<shared_ptr<QueryAnswer>, float>>& selected) {
    unsigned int size = population.size();
    unsigned int cursor1 = std::rand() % size;
    unsigned int cursor2 = std::rand() % size;
    if (cursor2 == cursor1) {
        cursor2 = (cursor2 + 1) % size;
    }
    unsigned int winner = (population[cursor1].second > population[cursor2].second ? cursor1 : cursor2);

    selected.push_back(population[winner]);
    population.erase(population.begin() + winner, population.begin() + winner + 1);
}

void QueryEvolutionProcessor::select_best_individuals(
    shared_ptr<QueryEvolutionProxy> proxy,
    vector<std::pair<shared_ptr<QueryAnswer>, float>>& population,
    vector<std::pair<shared_ptr<QueryAnswer>, float>>& selected) {
    double selection_rate = proxy->parameters.get<double>(QueryEvolutionProxy::SELECTION_RATE);
    unsigned int count = (unsigned int) std::lround(selection_rate * population.size());
    unsigned int population_size = population.size();

    if (count > 0) {
        if (count > population_size) {
            Utils::error("Invalid evolution parameters. Selection count: " + std::to_string(count) +
                         " population size: " + std::to_string(population_size));
        } else if (count == population_size) {
            selected.insert(selected.begin(), population.begin(), population.end());
            population.clear();
        }
        apply_elitism(proxy, population, selected);
        while (selected.size() < count) {
            select_one_by_tournament(proxy, population, selected);
        }
    }
    float sum = 0;
    for (unsigned int i = 0; i < count; i++) {
        LOG_INFO("Selected: " + (proxy->populate_metta_mapping(selected[i].first.get()),
                                 sum += selected[i].second,
                                 selected[i].first->metta_expression[selected[i].first->handles[0]] +
                                     " " + std::to_string(selected[i].first->strength)));
    }
    LOG_INFO("Generation: " + std::to_string(this->generation_count) +
             " - Average fitness in selected group: " + std::to_string(sum / count));
}

float eval_word(const string& handle, string& word) {
    auto db = AtomDBSingleton::get_instance();
    shared_ptr<atomdb_api_types::AtomDocument> word_document;
    shared_ptr<atomdb_api_types::AtomDocument> word_name_document;
    word_document = db->get_atom_document(handle);
    string symbol_handle = string(word_document->get("targets", 1));
    word_name_document = db->get_atom_document(symbol_handle);
    const char* word_name = word_name_document->get("name");
    word = string(word_name);
    unsigned int count = 0;
    unsigned int sentence_length = 0;
    for (unsigned int i = 0; word_name[i] != '\0'; i++) {
        if ((word_name[i] != ' ') && (word_name[i] != '"')) {
            sentence_length++;
        }
        if (word_name[i] == 'c') {
            count++;
        }
    }
    if (sentence_length == 0) {
        return 0;
    } else {
        return ((float) count) / ((float) sentence_length);
    }
}

/*
void QueryEvolutionProcessor::correlate_similar(shared_ptr<QueryEvolutionProxy> proxy,
                                                shared_ptr<QueryAnswer> correlation_query_answer) {
    vector<string> query_tokens;
    unsigned int cursor = 0;
    vector<string> original_tokens = proxy->get_correlation_tokens();
    set<string> variables = proxy->get_correlation_variables();
    unsigned int size = original_tokens.size();

    // Apply QueryAnswer's assignment to original tokens
    while (cursor < size) {
        string token = original_tokens[cursor++];
        if (token == LinkSchema::UNTYPED_VARIABLE) {
            if (cursor == size) {
                Utils::error("Invalid correlation_tokens");
                return;
            }
            token = original_tokens[cursor++];
            if (variables.find(token) != variables.end()) {
                string value = correlation_query_answer->assignment.get(token);
                query_tokens.push_back(LinkSchema::ATOM);
                query_tokens.push_back(value);
            } else {
                query_tokens.push_back(LinkSchema::UNTYPED_VARIABLE);
                query_tokens.push_back(token);
            }
        } else {
            query_tokens.push_back(token);
        }
    }

    // Update AttentionBroker
    set<string> handle_set;
    handle_set.insert(correlation_query_answer->assignment.get("sentence1"));
    auto pm_query = issue_correlation_query(proxy, query_tokens);
    while (!pm_query->finished()) {
        shared_ptr<QueryAnswer> answer = pm_query->pop();
        string word;
        if (answer != NULL) {
            if (eval_word(answer->assignment.get("word1"), word) >= correlation_query_answer->strength) {
                handle_set.insert(answer->assignment.get("sentence2"));
            }
        } else {
            Utils::sleep();
        }
    }
    AttentionBrokerClient::correlate(handle_set, proxy->get_context());
}
*/

void QueryEvolutionProcessor::correlate_similar(shared_ptr<QueryEvolutionProxy> proxy,
                                                shared_ptr<QueryAnswer> correlation_query_answer) {

    vector<string> query_tokens;
    unsigned int cursor = 0;
    vector<string> original_tokens = proxy->get_correlation_tokens();
    set<string> variables = proxy->get_correlation_variables();
    unsigned int size = original_tokens.size();

    // Apply QueryAnswer's assignment to original tokens
    while (cursor < size) {
        string token = original_tokens[cursor++];
        if (token == LinkSchema::UNTYPED_VARIABLE) {
            if (cursor == size) {
                Utils::error("Invalid correlation_tokens");
                return;
            }
            token = original_tokens[cursor++];
            if (variables.find(token) != variables.end()) {
                string value = correlation_query_answer->assignment.get(token);
                query_tokens.push_back(LinkSchema::ATOM);
                query_tokens.push_back(value);
            } else {
                query_tokens.push_back(LinkSchema::UNTYPED_VARIABLE);
                query_tokens.push_back(token);
            }
        } else {
            query_tokens.push_back(token);
        }
    }

    // Update AttentionBroker
    vector<string> handle_list;
    handle_list.push_back(correlation_query_answer->assignment.get("sentence1"));
    auto pm_query = issue_correlation_query(proxy, query_tokens);
    while (!pm_query->finished()) {
        shared_ptr<QueryAnswer> answer = pm_query->pop();
        string word;
        if (answer != NULL) {
            if (eval_word(answer->assignment.get("word1"), word) >= correlation_query_answer->strength) {
                LOG_DEBUG("Valid word: " + word);
                handle_list.push_back(answer->assignment.get("word1"));
            }
        } else {
            Utils::sleep();
        }
    }
    AttentionBrokerClient::asymmetric_correlate(handle_list, proxy->get_context());

    /*
    auto db = AtomDBSingleton::get_instance();
    vector<string> handle_list;
    bool error_flag = false;
    for (string handle : query_answer->handles) {
        shared_ptr<Atom> atom = db->get_atom(handle);
        handle_list.push_back(handle);
        if (atom->arity() == 3) { // XXX
            shared_ptr<Link> link = dynamic_pointer_cast<Link>(atom);
            string word_handle = link->targets[2];
            string word = link->targets[2];
            if (eval_word(word_handle, word) >= query_answer->strength) {
                handle_list.push_back(word_handle);
            }
        } else {
            error_flag = true;
            break;
        }
        AttentionBrokerClient::asymmetric_correlate(handle_list, proxy->get_context());
        handle_list.clear();
    }
    if (error_flag) {
        Utils::error("Invalid query answer: " query_answer->to_string());
    }

    // Update AttentionBroker
    handle_set.insert(correlation_query_answer->assignment.get("sentence1"));
    auto pm_query = issue_correlation_query(proxy, query_tokens);
    while (!pm_query->finished()) {
        shared_ptr<QueryAnswer> answer = pm_query->pop();
        string word;
        if (answer != NULL) {
            if (eval_word(answer->assignment.get("word1"), word) >= correlation_query_answer->strength) {
                handle_set.insert(answer->assignment.get("sentence2"));
            }
        } else {
            Utils::sleep();
        }
    }
    AttentionBrokerClient::correlate(handle_set, proxy->get_context());
    */
}

void QueryEvolutionProcessor::stimulate(shared_ptr<QueryEvolutionProxy> proxy,
                                        vector<std::pair<shared_ptr<QueryAnswer>, float>>& selected) {
    LOG_INFO("Stimulating " + std::to_string(selected.size()) + " selected QueryAnswers: ");
    unsigned int importance_tokens =
        proxy->parameters.get<unsigned int>(QueryEvolutionProxy::TOTAL_ATTENTION_TOKENS);

    map<string, unsigned int> handle_count;
    for (auto pair : selected) {
        unsigned int value = (unsigned int) std::lround(pair.second * importance_tokens);
        string handle = pair.first->assignment.get("sentence1");
        if (handle_count.find(handle) == handle_count.end()) {
            handle_count[handle] = value;
        } else {
            if (value > handle_count[handle]) {
                handle_count[handle] = value;
            }
        }
    }
    /*
    for (auto pair : selected) {
        for (string handle : pair.first->handles) {
            unsigned int value = (unsigned int) std::lround(pair.second * importance_tokens);
            if (handle_count.find(handle) == handle_count.end()) {
                handle_count[handle] = value;
            } else {
                if (value > handle_count[handle]) {
                    handle_count[handle] = value;
                }
            }
        }
    }
    */
    AttentionBrokerClient::stimulate(handle_count, proxy->get_context());
}

void QueryEvolutionProcessor::update_attention_allocation(
    shared_ptr<QueryEvolutionProxy> proxy, vector<std::pair<shared_ptr<QueryAnswer>, float>>& selected) {
    if (proxy->parameters.get<bool>(BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS)) {
        return;
    }
    unsigned int count = 1;
    for (auto pair : selected) {
        LOG_INFO("Correlating QueryAnswer (" + std::to_string(count) + "/" +
                 std::to_string(selected.size()) + "): " + pair.first->to_string());
        correlate_similar(proxy, pair.first);
        count++;
    }
    stimulate(proxy, selected);
}

void QueryEvolutionProcessor::evolve_query(shared_ptr<StoppableThread> monitor,
                                           shared_ptr<QueryEvolutionProxy> proxy) {
    vector<std::pair<shared_ptr<QueryAnswer>, float>> population;
    vector<std::pair<shared_ptr<QueryAnswer>, float>> selected;
    this->generation_count = 1;
    RAM_FOOTPRINT_START(evolution);
    STOP_WATCH_START(evolution);
    while (!monitor->stopped() && !proxy->stop_criteria_met()) {
        RAM_CHECKPOINT("Generation " + std::to_string(this->generation_count));
        STOP_WATCH_START(generation);
        STOP_WATCH_START(sample_population);
        sample_population(monitor, proxy, population);
        STOP_WATCH_FINISH(sample_population, "EvolutionPopulationSampling");
        LOG_INFO("========== Generation: " + std::to_string(this->generation_count) + ". Sampled " +
                 std::to_string(population.size()) + " individuals. ==========");
        proxy->new_population_sampled(population);
        if ((population.size() > 0) && !proxy->stop_criteria_met()) {
            proxy->flush_answer_bundle();
            STOP_WATCH_START(selection);
            select_best_individuals(proxy, population, selected);
            STOP_WATCH_FINISH(selection, "EvolutionIndividualSelection");
            LOG_INFO("Selected " + std::to_string(selected.size()) +
                     " individuals to update attention allocation.");
            if (selected.size() > 0) {
                STOP_WATCH_START(attention_broker);
                update_attention_allocation(proxy, selected);
                STOP_WATCH_FINISH(attention_broker, "EvolutionAttentionUpdate");
            }
            population.clear();
            selected.clear();
        }
        RAM_FOOTPRINT_CHECK(evolution, "Generation " + std::to_string(this->generation_count));
        STOP_WATCH_FINISH(generation, "OneGeneration");
        this->generation_count++;
    }
    proxy->flush_answer_bundle();
    LOG_INFO("Total number of visited individuals: " + std::to_string(this->visited_individuals.size()));
    LOG_INFO("Average renew rate per generation: " + std::to_string((double) this->visited_individuals.size() / ((double) proxy->parameters.get<unsigned int>(QueryEvolutionProxy::POPULATION_SIZE) * (this->generation_count - 1))));
    STOP_WATCH_FINISH(evolution, "QueryEvolution");
    RAM_FOOTPRINT_FINISH(evolution, "");
    Utils::sleep(1000);
    proxy->query_processing_finished();
}

void QueryEvolutionProcessor::remove_query_thread(const string& stoppable_thread_id) {
    lock_guard<mutex> semaphore(this->query_threads_mutex);
    this->query_threads.erase(this->query_threads.find(stoppable_thread_id));
}
