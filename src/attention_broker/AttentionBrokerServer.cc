#include "AttentionBrokerServer.h"

#include "RequestSelector.h"

using namespace attention_broker_server;

// --------------------------------------------------------------------------------
// Public methods

AttentionBrokerServer::AttentionBrokerServer()
    : stimulus_requests(new SharedQueue()),
      correlation_requests(new SharedQueue()),
      worker_threads(
          new WorkerThreads(stimulus_requests, correlation_requests)),
      updater(HebbianNetworkUpdater::factory(
          HebbianNetworkUpdaterType::EXACT_COUNT)),
      stimulus_spreader(
          StimulusSpreader::factory(StimulusSpreaderType::TOKEN)) {

  auto *network = new HebbianNetwork();
  this->hebbian_network[this->global_context] = network;
}

AttentionBrokerServer::~AttentionBrokerServer() {
  graceful_shutdown();
  delete this->worker_threads;
  delete this->stimulus_requests;
  delete this->correlation_requests;
  delete this->updater;
  delete this->stimulus_spreader;
  for (const auto &pair : this->hebbian_network) {
    delete pair.second;
  }
}

void AttentionBrokerServer::graceful_shutdown() {
  this->rpc_api_enabled = false;
  this->worker_threads->graceful_stop();
}

// RPC API

Status AttentionBrokerServer::ping(ServerContext * /*grpc_context*/,
                                   const dasproto::Empty * /*request*/,
                                   dasproto::Ack *reply) {
  reply->set_msg("PING");
  if (rpc_api_enabled) {
    return Status::OK;
  } else {
    return Status::CANCELLED;
  }
}

Status AttentionBrokerServer::stimulate(ServerContext * /*grpc_context*/,
                                        const dasproto::HandleCount *request,
                                        dasproto::Ack *reply) {
#ifdef DEBUG
  cout << "AttentionBrokerServer::stimulate() BEGIN" << endl;
  cout << "Context: " << request->context() << endl;
#endif
  if (request->map_size() > 0) {
    HebbianNetwork *network = select_hebbian_network(request->context());
    ((dasproto::HandleCount *)request)->set_hebbian_network((long)network);
    // this->stimulus_requests->enqueue((void *) request);
    this->stimulus_spreader->spread_stimuli(request);
  }
  reply->set_msg("STIMULATE");
#ifdef DEBUG
  cout << "AttentionBrokerServer::stimulate() END" << endl;
#endif
  if (rpc_api_enabled) {
    return Status::OK;
  } else {
    return Status::CANCELLED;
  }
}

Status AttentionBrokerServer::correlate(ServerContext * /*grpc_context*/,
                                        const dasproto::HandleList *request,
                                        dasproto::Ack *reply) {
#ifdef DEBUG
  cout << "AttentionBrokerServer::correlate() BEGIN" << endl;
  cout << "Context: " << request->context() << endl;
#endif
  if (request->list_size() > 0) {
    HebbianNetwork *network = select_hebbian_network(request->context());
    ((dasproto::HandleList *)request)->set_hebbian_network((long)network);
    // this->correlation_requests->enqueue((void *) request);
    this->updater->correlation(request);
  }
  reply->set_msg("CORRELATE");
#ifdef DEBUG
  cout << "AttentionBrokerServer::correlate() END" << endl;
#endif
  if (rpc_api_enabled) {
    return Status::OK;
  } else {
    return Status::CANCELLED;
  }
}

Status
AttentionBrokerServer::get_importance(ServerContext * /*grpc_context*/,
                                      const dasproto::HandleList *request,
                                      dasproto::ImportanceList *reply) {
#ifdef DEBUG
  cout << "AttentionBrokerServer::get_importance() BEGIN" << endl;
  cout << "Context: " << request->context() << endl;
#endif
  if (this->rpc_api_enabled) {
    int const num_handles = request->list_size();
    if (num_handles > 0) {
      HebbianNetwork *network = select_hebbian_network(request->context());
      for (int i = 0; i < num_handles; i++) {
        float const importance = network->get_node_importance(request->list(i));
        reply->add_list(importance);
      }
    }
#ifdef DEBUG
    cout << "AttentionBrokerServer::get_importance() END" << endl;
#endif
    return Status::OK;
  } else {
    return Status::CANCELLED;
  }
}

// --------------------------------------------------------------------------------
// Private methods
//

HebbianNetwork *
AttentionBrokerServer::select_hebbian_network(const string &context) {
  HebbianNetwork *network = nullptr;
  if ((!context.empty()) &&
      (this->hebbian_network.find(context) != this->hebbian_network.end())) {
    network = this->hebbian_network[context];
  }
  if (context.empty()) {
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
