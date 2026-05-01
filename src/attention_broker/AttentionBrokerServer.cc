#include "AttentionBrokerServer.h"

#include "RequestSelector.h"

#define LOG_LEVEL DEBUG_LEVEL
#include "Logger.h"

using namespace attention_broker;

double AttentionBrokerServer::RENT_RATE = 0.75;
double AttentionBrokerServer::SPREADING_RATE_LOWERBOUND = 0.10;
double AttentionBrokerServer::SPREADING_RATE_UPPERBOUND = 0.10;

// --------------------------------------------------------------------------------
// Public methods

AttentionBrokerServer::AttentionBrokerServer() {
    this->global_context = "global";
    this->stimulus_requests = new SharedQueue();
    this->correlation_requests = new SharedQueue();
    this->worker_threads = new WorkerThreads(stimulus_requests, correlation_requests);
    HebbianNetwork* network = new HebbianNetwork();
    this->hebbian_network[this->global_context] = network;
    this->updater = HebbianNetworkUpdater::factory(HebbianNetworkUpdaterType::EXACT_COUNT);
    this->stimulus_spreader = StimulusSpreader::factory(StimulusSpreaderType::TOKEN);
}

AttentionBrokerServer::~AttentionBrokerServer() {
    graceful_shutdown();
    delete this->worker_threads;
    delete this->stimulus_requests;
    delete this->correlation_requests;
    delete this->updater;
    delete this->stimulus_spreader;
    for (auto pair : this->hebbian_network) {
        delete pair.second;
    }
}

void AttentionBrokerServer::graceful_shutdown() {
    this->rpc_api_enabled = false;
    this->worker_threads->graceful_stop();
}

// RPC API

Status AttentionBrokerServer::ping(ServerContext* grpc_context,
                                   const dasproto::Empty* request,
                                   dasproto::Ack* reply) {
    reply->set_msg("PING");
    if (rpc_api_enabled) {
        return Status::OK;
    } else {
        return Status::CANCELLED;
    }
}

Status AttentionBrokerServer::stimulate(ServerContext* grpc_context,
                                        const dasproto::HandleCount* request,
                                        dasproto::Ack* reply) {
    LOG_INFO("Stimulating " << (request->map_size() - 1)
                            << " handles in context: '" + request->context() + "'");
    if (request->map_size() > 1) {
        HebbianNetwork* network = select_hebbian_network(request->context());
        ((dasproto::HandleCount*) request)->set_hebbian_network((long) network);
        // this->stimulus_requests->enqueue((void *) request);
        this->stimulus_spreader->spread_stimuli(request);
    }
    reply->set_msg("STIMULATE");
#if LOG_LEVEL >= DEBUG_LEVEL
    HebbianNetwork* hebbian_network = select_hebbian_network(request->context());
    for (auto pair : request->map()) {
        if (pair.first != "SUM") {
            LOG_DEBUG(pair.first + ": " + std::to_string(pair.second) + " -> " +
                      std::to_string(hebbian_network->get_node_importance(pair.first)));
        }
    }
    auto iterator = request->map().find("SUM");
    unsigned int total_wages = iterator->second;
    LOG_DEBUG("SUM: " + std::to_string(total_wages));
#endif
    if (rpc_api_enabled) {
        return Status::OK;
    } else {
        LOG_ERROR("AttentionBrokerServer::stimulate() failed");
        return Status::CANCELLED;
    }
}

Status AttentionBrokerServer::correlate(ServerContext* grpc_context,
                                        const dasproto::HandleList* request,
                                        dasproto::Ack* reply) {
    LOG_INFO("Correlating " << request->list_size()
                            << " handles in context: '" + request->context() + "'");
    if (request->list_size() > 1) {
        HebbianNetwork* network = select_hebbian_network(request->context());
        ((dasproto::HandleList*) request)->set_hebbian_network((long) network);
        // this->correlation_requests->enqueue((void *) request);
        this->updater->correlation(request);
    } else {
        LOG_INFO("Discarding invalid correlation request with too few arguments.");
    }
    reply->set_msg("CORRELATE");
    if (rpc_api_enabled) {
        return Status::OK;
    } else {
        LOG_ERROR("AttentionBrokerServer::correlate() failed");
        return Status::CANCELLED;
    }
}

Status AttentionBrokerServer::asymmetric_correlate(ServerContext* grpc_context,
                                                   const dasproto::HandleList* request,
                                                   dasproto::Ack* reply) {
    LOG_INFO("Correlating (asymmetric) " << request->list_size()
                                         << " handles in context: '" + request->context() + "'");
    if (request->list_size() > 1) {
        HebbianNetwork* network = select_hebbian_network(request->context());
        ((dasproto::HandleList*) request)->set_hebbian_network((long) network);
        // this->correlation_requests->enqueue((void *) request);
        this->updater->asymmetric_correlation(request);
    } else {
        LOG_INFO("Discarding invalid correlation (asymmetric) request with too few arguments.");
    }
    reply->set_msg("ASYMMETRIC_CORRELATE");
    if (rpc_api_enabled) {
        return Status::OK;
    } else {
        LOG_ERROR("AttentionBrokerServer::asymmetric_correlate() failed");
        return Status::CANCELLED;
    }
}

Status AttentionBrokerServer::get_importance(ServerContext* grpc_context,
                                             const dasproto::HandleList* request,
                                             dasproto::ImportanceList* reply) {
    LOG_INFO("Getting importance of " << request->list_size()
                                      << " handles in context: '" + request->context() + "'");
    unsigned int count_positive = 0;
    if (this->rpc_api_enabled) {
        int num_handles = request->list_size();
        if (num_handles > 0) {
            HebbianNetwork* network = select_hebbian_network(request->context());
            for (int i = 0; i < num_handles; i++) {
                float importance = network->get_node_importance(request->list(i));
                if (importance > 0) {
                    LOG_DEBUG("P " + request->list(i) + ": " + std::to_string(importance));
                    count_positive++;
                } else {
                    LOG_DEBUG("Z " + request->list(i) + ": " + std::to_string(importance));
                }
                reply->add_list(importance);
            }
            LOG_INFO(std::to_string(count_positive) + " positive values");
        }
        return Status::OK;
    } else {
        LOG_ERROR("AttentionBrokerServer::get_importance() failed");
        return Status::CANCELLED;
    }
}

Status AttentionBrokerServer::set_determiners(ServerContext* grpc_context,
                                              const dasproto::HandleListList* request,
                                              dasproto::Ack* reply) {
    LOG_INFO("Setting determiners for " << request->list_size()
                                        << " handles in context: '" + request->context() + "'");
    if (request->list_size() > 0) {
        HebbianNetwork* network = select_hebbian_network(request->context());
        for (int i = 0; i < request->list_size(); i++) {
            if (request->list(i).list_size() > 1) {
                this->updater->determiners(request->list(i), network);
            } else {
                LOG_INFO("Discarding invalid determiners setting request with too few arguments.");
            }
        }
    }
    reply->set_msg("SET_DETERMINERS");
    if (rpc_api_enabled) {
        return Status::OK;
    } else {
        LOG_ERROR("AttentionBrokerServer::set_determiners() failed");
        return Status::CANCELLED;
    }
}

Status AttentionBrokerServer::set_parameters(ServerContext* grpc_context,
                                             const dasproto::Parameters* request,
                                             dasproto::Ack* reply) {
    LOG_INFO("Setting dynamics parameters in all contexts. RENT_RATE: "
             << request->rent_rate()
             << " SPREADING_RATE_LOWERBOUND: " << request->spreading_rate_lowerbound()
             << " SPREADING_RATE_UPPERBOUND: " << request->spreading_rate_upperbound());
    RENT_RATE = request->rent_rate();
    SPREADING_RATE_LOWERBOUND = request->spreading_rate_lowerbound();
    SPREADING_RATE_UPPERBOUND = request->spreading_rate_upperbound();
    reply->set_msg("SET_PARAMETERS");
    if (rpc_api_enabled) {
        return Status::OK;
    } else {
        LOG_ERROR("AttentionBrokerServer::set_parameters() failed");
        return Status::CANCELLED;
    }
}

/**
 * Note to revisor and developers:
 *
 * This dead version of save_context() and drop_and_load_context() uses serialization instead of
 * a text file with update commands. I (Andre Senna) was working on this just before one of the
 * AGI meetups and I remember this serialization/deserialization approach hit a blocker
 * but I don't remember what exactly it is and unfortunetely I didn't documented it.
 * So I'm keeping this version of these methods here while I make another
 * version which uses a text file of update commands. The serialization/deserialization
 * approach may still be useful in order to save STI values as well as hebboan links and
 * determiners.
 *
Status AttentionBrokerServer::save_context(ServerContext* grpc_context,
                                           const dasproto::ContextPersistence* request,
                                           dasproto::Ack* reply) {
    LOG_INFO("Saving contents of context: " + request->context() +
             " into file: " + request->file_name());
    if (this->rpc_api_enabled) {
        HebbianNetwork* network = select_hebbian_network(request->context());
        ofstream file(request->file_name());
        if (!file.is_open()) {
            LOG_ERROR("Couldn't open file: " + request->file_name());
            return Status::CANCELLED;
        }
        try {
            network->serialize(file);
            file.close();
        } catch (const std::exception& e) {
            LOG_ERROR("Error in context serialization using file: " + request->file_name());
            return Status::CANCELLED;
        }
        reply->set_msg("SAVE_CONTEXT");
        return Status::OK;
    } else {
        LOG_ERROR("AttentionBrokerServer::save_context() failed");
        return Status::CANCELLED;
    }
}

Status AttentionBrokerServer::drop_and_load_context(ServerContext* grpc_context,
                                                    const dasproto::ContextPersistence* request,
                                                    dasproto::Ack* reply) {
    LOG_INFO("Dropping contents of context: " + request->context() +
             " Reading from file: " + request->file_name());
    if (this->rpc_api_enabled) {
        HebbianNetwork* network = select_hebbian_network(request->context());
        ifstream file(request->file_name());
        if (!file.is_open()) {
            LOG_ERROR("Couldn't open file: " + request->file_name());
            return Status::CANCELLED;
        }
        network->clear();
        try {
            network->deserialize(file);
            file.close();
        } catch (const std::exception& e) {
            LOG_ERROR("Error in context deserialization using file: " + request->file_name());
            return Status::CANCELLED;
        }
        reply->set_msg("DROP_AND_LOAD_CONTEXT");
        return Status::OK;
    } else {
        LOG_ERROR("AttentionBrokerServer::drop_and_load_context() failed");
        return Status::CANCELLED;
    }
}
*/

Status AttentionBrokerServer::save_context(ServerContext* grpc_context,
                                           const dasproto::ContextPersistence* request,
                                           dasproto::Ack* reply) {
    Utils::error("AttentionBrokerServer::save_context() is not implemented");
    return Status::CANCELLED;
}

Status AttentionBrokerServer::drop_and_load_context(ServerContext* grpc_context,
                                                    const dasproto::ContextPersistence* request,
                                                    dasproto::Ack* reply) {

    string context = request->context();
    string file_name = request->file_name();
    HebbianNetwork* network = select_hebbian_network(context);
    ifstream file(file_name);
    if (!file.is_open()) {
        LOG_ERROR("Couldn't open file: " + file_name);
        return Status::CANCELLED;
    }
    LOG_DEBUG("Reading context from file: " + file_name);
    network->clear();
    dasproto::HandleList correlation_request;
    dasproto::HandleCount activation_request;
    dasproto::HandleList determiner_request;
    correlation_request.set_context(context);
    activation_request.set_context(context);
    determiner_request.set_context(context);
    unsigned int sum_activation = 0;
    vector<string> line;
    unsigned int line_count = 0;
    try {
        while (Utils::read_and_split(line, file)) {
            line_count++;
            if (line.size() < 2) {
                Utils::error("Invalid context command with no arguments in line " + std::to_string(line_count) + " of file " + file_name);
            }
            if (line[0] == "DET") {
                for (unsigned int i = 1; i < line.size(); i++) {
                    determiner_request.add_list(line[i]);
                }
                LOG_DEBUG("Adding " + std::to_string(line.size() - 2) + " determiners for " + line[1]);
                this->updater->determiners(determiner_request, network);
                determiner_request.clear_list();
            } else if (line[0] == "COR") {
                for (unsigned int i = 1; i < line.size(); i++) {
                    correlation_request.add_list(line[i]);
                }
                LOG_DEBUG("Correlating " + std::to_string(line.size() - 2) + " handles with " + line[1]);
                this->updater->asymmetric_correlation(&correlation_request);
                correlation_request.clear_list();
            } else if (line[0] == "ACT") {
                for (unsigned int i = 1; i < line.size(); i++) {
                    if (activation_request.map().count(line[i]) > 0) {
                        (*activation_request.mutable_map())[line[i]] = 1;
                    } else {
                        (*activation_request.mutable_map())[line[i]] = (*activation_request.mutable_map())[line[i]] + 1;
                    }
                    sum_activation++;
                }
            } else {
                Utils::error("Invalid context mnemonic: " + line[0] + " in context file: " + file_name);
            }
            line.clear();
        }
        if (sum_activation > 0) {
            (*activation_request.mutable_map())["SUM"] = sum_activation;
            LOG_DEBUG("Spreading " + std::to_string(sum_activation) + " tokens of activation");
            this->stimulus_spreader->spread_stimuli(&activation_request);
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error processing context file: " + file_name);
        return Status::CANCELLED;
    }
    return Status::OK;
}

// --------------------------------------------------------------------------------
// Private methods
//

HebbianNetwork* AttentionBrokerServer::select_hebbian_network(const string& context) {
    HebbianNetwork* network;
    if ((context != "") && (this->hebbian_network.find(context) != this->hebbian_network.end())) {
        network = this->hebbian_network[context];
    }
    if (context == "") {
        network = this->hebbian_network[this->global_context];
    } else {
        if (this->hebbian_network.find(context) == this->hebbian_network.end()) {
            network = new HebbianNetwork();
            this->hebbian_network[context] = network;
        } else {
            network = this->hebbian_network[context];
        }
    }
    return network;
}
