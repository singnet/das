#include "QueryEvolutionProcessor.h"

#include "AttentionBrokerClient.h"
#include "Hasher.h"
#include "LinkSchema.h"
#include "QueryEvolutionProxy.h"
#include "ServiceBus.h"
#include "ServiceBusSingleton.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace evolution;
using namespace query_engine;
using namespace atoms;
using namespace service_bus;
using namespace attention_broker;

// -------------------------------------------------------------------------------------------------
// Constructors and destructors

QueryEvolutionProcessor::QueryEvolutionProcessor() : BusCommandProcessor({ServiceBus::QUERY_EVOLUTION}) {
    AttentionBrokerClient::health_check(true);
}

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
        RAISE_ERROR("Invalid thread id: " + thread_id);
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
            RAISE_ERROR("Syntax error in query command. Missing implicit parameters.");
        }
        proxy->untokenize(proxy->args);
        string command = proxy->get_command();
        if (command == ServiceBus::QUERY_EVOLUTION) {
            LOG_INFO("Proxy: " << proxy->to_string());
            this->evolve_query(monitor, proxy);
        } else {
            RAISE_ERROR("Invalid command " + command + " in QueryEvolutionProcessor");
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
    auto pm_proxy = make_shared<PatternMatchingQueryProxy>(proxy->get_query_tokens(), proxy->get_context());
    pm_proxy->parameters = proxy->parameters;
    pm_proxy->parameters[BaseQueryProxy::ATTENTION_CORRELATION] = (unsigned int) BaseQueryProxy::NONE;
    pm_proxy->parameters[BaseQueryProxy::ATTENTION_UPDATE] = (unsigned int) BaseQueryProxy::NONE;
    pm_proxy->parameters[BaseQueryProxy::POPULATE_METTA_MAPPING] = proxy->parameters.get<bool>(BaseQueryProxy::POPULATE_METTA_MAPPING) || proxy->parameters.get<bool>(BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS);  // enforced when MeTTa queries are being used
    pm_proxy->parameters[PatternMatchingQueryProxy::MAX_ANSWERS] = proxy->parameters.get<unsigned int>(QueryEvolutionProxy::POPULATION_SIZE);
    ServiceBusSingleton::get_instance()->issue_bus_command(pm_proxy);
    return pm_proxy;
}

shared_ptr<PatternMatchingQueryProxy> QueryEvolutionProcessor::issue_correlation_query(
    shared_ptr<QueryEvolutionProxy> proxy, vector<string> query_tokens) {
    auto pm_proxy = make_shared<PatternMatchingQueryProxy>(query_tokens, proxy->get_context());
    pm_proxy->parameters = proxy->parameters;
    pm_proxy->parameters[BaseQueryProxy::UNIQUE_ASSIGNMENT_FLAG] = true;
    pm_proxy->parameters[BaseQueryProxy::ATTENTION_CORRELATION] = (unsigned int) BaseQueryProxy::NONE;
    pm_proxy->parameters[BaseQueryProxy::ATTENTION_UPDATE] = (unsigned int) BaseQueryProxy::NONE;
    pm_proxy->parameters[BaseQueryProxy::USE_LINK_TEMPLATE_CACHE] = false;
    pm_proxy->parameters[PatternMatchingQueryProxy::POSITIVE_IMPORTANCE_FLAG] = true;  //(this->generation_count > 1);
    pm_proxy->parameters[PatternMatchingQueryProxy::DISREGARD_IMPORTANCE_FLAG] = false;
    pm_proxy->parameters[PatternMatchingQueryProxy::UNIQUE_VALUE_FLAG] = false;
    pm_proxy->parameters[PatternMatchingQueryProxy::MAX_ANSWERS] = proxy->parameters.get<unsigned int>(QueryEvolutionProxy::POPULATION_SIZE) * 5;
    ServiceBusSingleton::get_instance()->issue_bus_command(pm_proxy);
    return pm_proxy;
}

void QueryEvolutionProcessor::sample_population(
    shared_ptr<StoppableThread> monitor,
    shared_ptr<QueryEvolutionProxy> proxy,
    vector<std::pair<shared_ptr<QueryAnswer>, float>>& population) {
    bool remote_fitness = proxy->is_fitness_function_remote();
    vector<string> answer_bundle_vector;
    unsigned int population_size = proxy->parameters.get<unsigned int>(QueryEvolutionProxy::POPULATION_SIZE);
    bool metta_mapping = proxy->parameters.get<bool>(BaseQueryProxy::POPULATE_METTA_MAPPING);

    auto pm_query = issue_sampling_query(proxy);

    unsigned int positive_importance_count = 0;
    unsigned int visited_count = this->visited_individuals.size();
    while ((!pm_query->finished()) && (!monitor->stopped()) && (population.size() < population_size)) {
        shared_ptr<QueryAnswer> answer = pm_query->pop();
        if (answer != NULL) {
            float fitness;
            if (remote_fitness) {
                fitness = 0;
                if (! metta_mapping) {
                    // If PM agent didn't populate metta mapping as per parameter, we need to enforce
                    // these mappings get populated here, before calling remote fitness evaluation.
                    proxy->populate_metta_mapping(answer.get());
                }
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
            this->visited_individuals.insert(answer->compute_hash());
        } else {
            Utils::sleep();
        }
    }
    double renew_rate = ((double) this->visited_individuals.size() - visited_count) / population_size;
    LOG_INFO("Individuals with non-zero importance: " + std::to_string(positive_importance_count));
    LOG_INFO("Renew rate: " + std::to_string(renew_rate));
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
            RAISE_ERROR(
                "Expected POPULATION_SIZE <= MAX_BUNDLE_SIZE in order to use remote fitness evaluation");
            return;
        }
        proxy->remote_fitness_evaluation(answer_bundle_vector);
        while (!proxy->remote_fitness_evaluation_finished()) {
            Utils::sleep();
        }
        vector<float> fitness_bundle = proxy->get_remotely_evaluated_fitness();
        if (fitness_bundle.size() != population_size) {
            RAISE_ERROR("Invalid fitness bundle of size: " + std::to_string(fitness_bundle.size()));
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
            RAISE_ERROR("Invalid evolution parameters. Elitism count: " + std::to_string(count) +
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
    bool metta_mapping = proxy->parameters.get<bool>(BaseQueryProxy::POPULATE_METTA_MAPPING);
    unsigned int count = (unsigned int) std::lround(selection_rate * population.size());
    unsigned int population_size = population.size();

    if (count > 0) {
        if (count > population_size) {
            RAISE_ERROR("Invalid evolution parameters. Selection count: " + std::to_string(count) +
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
        sum += selected[i].second;
#if LOG_LEVEL >= DEBUG_LEVEL
        if (! metta_mapping) {
            proxy->populate_metta_mapping(selected[i].first.get());
        }
        LOG_DEBUG("Selected: [" + std::to_string(selected[i].first->strength) + "] " + selected[i].first->to_string(true));
#endif
    }
    LOG_INFO("Average fitness in selected group: " + std::to_string(sum / count));
}

void QueryEvolutionProcessor::correlate_similar(shared_ptr<QueryEvolutionProxy> proxy,
                                                shared_ptr<QueryAnswer> correlation_query_answer) {
    vector<string> query_tokens;
    map<string, vector<string>> handle_lists;
    vector<vector<string>> correlation_queries = proxy->get_correlation_queries();
    vector<map<string, QueryAnswerElement>> correlation_replacements =
        proxy->get_correlation_replacements();
    vector<pair<QueryAnswerElement, QueryAnswerElement>> correlation_mappings =
        proxy->get_correlation_mappings();
    if (correlation_queries.size() != correlation_replacements.size()) {
        RAISE_ERROR("Invalid correlation queries/replacements. Proxy: " + proxy->to_string());
    }
    if (correlation_mappings.size() == 0) {
        RAISE_ERROR("Invalid correlation mappings. Proxy: " + proxy->to_string());
    }

    for (unsigned int i = 0; i < correlation_queries.size(); i++) {
        // Rewrite correlation query using handles from original query answer
        query_tokens.clear();
        if (proxy->parameters.get<bool>(BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS)) {
            if (correlation_queries[i].size() != 1) {
                RAISE_ERROR("Invalid MeTTa expression as correlation query");
                return;
            }
            string expression = correlation_queries[i][0];
            for (auto pair : correlation_replacements[i]) {
                string atom_handle = correlation_query_answer->get(pair.second);
                Utils::replace_all(expression,
                                   "$" + pair.first,
                                   correlation_query_answer->metta_expression[atom_handle]);
            }
            query_tokens = {expression};
        } else {
            unsigned int cursor = 0;
            unsigned int size = correlation_queries[i].size();

            // Apply QueryAnswer's assignment to original tokens
            while (cursor < size) {
                string token = correlation_queries[i][cursor++];
                if (token == LinkSchema::UNTYPED_VARIABLE) {
                    if (cursor == size) {
                        RAISE_ERROR("Invalid correlation_tokens");
                        return;
                    }
                    token = correlation_queries[i][cursor++];
                    if (correlation_replacements[i].find(token) != correlation_replacements[i].end()) {
                        string value = correlation_query_answer->get(correlation_replacements[i][token]);
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
        }

        // Update AttentionBroker
        handle_lists.clear();
        auto pm_query = issue_correlation_query(proxy, query_tokens);
        while (!pm_query->finished()) {
            shared_ptr<QueryAnswer> answer = pm_query->pop();
            string word;
            if (answer != NULL) {
                for (auto pair : correlation_mappings) {
                    string handle_original_answer = correlation_query_answer->get(pair.first, true);
                    string handle_correlation_answer = answer->get(pair.second, true);
                    if ((handle_original_answer != "") && (handle_correlation_answer != "")) {
                        if (handle_lists.find(handle_original_answer) == handle_lists.end()) {
                            handle_lists[handle_original_answer] = {handle_original_answer};
                        }
                        handle_lists[handle_original_answer].push_back(handle_correlation_answer);
                    }
                }
            } else {
                Utils::sleep();
            }
        }
        for (auto pair : handle_lists) {
            AttentionBrokerClient::asymmetric_correlate(pair.second, proxy->get_context());
        }
    }
}

void QueryEvolutionProcessor::stimulate(shared_ptr<QueryEvolutionProxy> proxy,
                                        vector<std::pair<shared_ptr<QueryAnswer>, float>>& selected) {
    LOG_INFO("Stimulating " + std::to_string(selected.size()) + " selected QueryAnswers");
    vector<pair<QueryAnswerElement, QueryAnswerElement>> correlation_mappings =
        proxy->get_correlation_mappings();
    if (correlation_mappings.size() == 0) {
        RAISE_ERROR("Invalid correlation mappings. Proxy: " + proxy->to_string());
    }

    float min_fitness = 1;
    for (auto pair : selected) {
        if (pair.second < min_fitness) {
            min_fitness = pair.second;
        }
    }
    if (min_fitness < MIN_SELECTED_FITNESS) {
        min_fitness = MIN_SELECTED_FITNESS;
        LOG_ERROR("Minimal fitness if below MIN_SELECTED_FITNESS");
    }

    float fitness_rate = (1.0 / min_fitness);

    map<string, unsigned int> handle_count;
    auto db = AtomDBSingleton::get_instance();
    for (auto pair : selected) {
        LOG_DEBUG("Selected answer: " + pair->first->to_string(proxy->parameters.get<bool>(BaseQueryProxy::POPULATE_METTA_MAPPING)));
        float v = (pair.second > MIN_SELECTED_FITNESS ? pair.second : MIN_SELECTED_FITNESS);
        unsigned int value = (unsigned int) std::lround(v * fitness_rate);
        set<string> node_handles;
        for (string handle : pair.first->get_handles_vector()) {
            //db->reachable_terminal_set(node_handles, handle, true);
            //node_handles.insert(handle);
        }
        for (auto correlation_pair : correlation_mappings) {
            node_handles.insert(pair.first->get(correlation_pair.first, true));
        }
        for (unsigned int i = 0; i < pair.first->get_paths_size(); i++) {
            for (string handle : pair.first->get_path_vector(i)) {
                node_handles.insert(handle);
                //db->reachable_terminal_set(node_handles, handle, true);
            }
        }
        for (string handle : node_handles) {
            LOG_DEBUG("Picked to stimulate: " + db->get_atom(handle)->to_string());
            unsigned int old_value = handle_count[handle];
            if (value > old_value) {
                handle_count[handle] = value;
            }
        }
    }

    AttentionBrokerClient::stimulate(handle_count, proxy->get_context());
}

void QueryEvolutionProcessor::update_attention_allocation(
    shared_ptr<QueryEvolutionProxy> proxy, vector<std::pair<shared_ptr<QueryAnswer>, float>>& selected) {
    unsigned int count = 1;
    for (auto pair : selected) {
        LOG_DEBUG("Correlating QueryAnswer (" + std::to_string(count) + "/" + std::to_string(selected.size()) + ")");
        correlate_similar(proxy, pair.first);
        count++;
    }
    stimulate(proxy, selected);
}

void QueryEvolutionProcessor::evolve_query(shared_ptr<StoppableThread> monitor,
                                           shared_ptr<QueryEvolutionProxy> proxy) {
    bool metta_flag = (proxy->get_query_tokens().size() == 1);
    for (auto correlation_query : proxy->get_correlation_queries()) {
        if ((correlation_query.size() == 1) != metta_flag) {
            RAISE_ERROR("Evolution and correlation queries should be either both tokens or both MeTTa.");
        }
    }
    vector<std::pair<shared_ptr<QueryAnswer>, float>> population;
    vector<std::pair<shared_ptr<QueryAnswer>, float>> selected;
    this->generation_count = 1;
    this->visited_individuals.clear();
    RAM_FOOTPRINT_START(evolution);
    STOP_WATCH_START(evolution);
    while (!monitor->stopped() && !proxy->stop_criteria_met()) {
        LOG_INFO("==================== Generation: " + std::to_string(this->generation_count) + " ====================");
        RAM_CHECKPOINT("Generation " + std::to_string(this->generation_count));
        STOP_WATCH_START(generation);
        STOP_WATCH_START(sample_population);
        sample_population(monitor, proxy, population);
        STOP_WATCH_FINISH(sample_population, "EvolutionPopulationSampling");
        LOG_INFO("Sampled " + std::to_string(population.size()) + " individuals.");
        proxy->new_population_sampled(population);
        if (population.size() > 0) {
            proxy->flush_answer_bundle();
            STOP_WATCH_START(selection);
            select_best_individuals(proxy, population, selected);
            STOP_WATCH_FINISH(selection, "EvolutionIndividualSelection");
            LOG_INFO("Selected " + std::to_string(selected.size()) + " individuals to update attention allocation.");
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
    LOG_INFO("Average renew rate per generation: " +
             std::to_string(
                 (double) this->visited_individuals.size() /
                 ((double) proxy->parameters.get<unsigned int>(QueryEvolutionProxy::POPULATION_SIZE) *
                  (this->generation_count - 1))));
    STOP_WATCH_FINISH(evolution, "QueryEvolution");
    RAM_FOOTPRINT_FINISH(evolution, "");
    Utils::sleep(1000);
    proxy->query_processing_finished();
}

void QueryEvolutionProcessor::remove_query_thread(const string& stoppable_thread_id) {
    lock_guard<mutex> semaphore(this->query_threads_mutex);
    this->query_threads.erase(this->query_threads.find(stoppable_thread_id));
}
