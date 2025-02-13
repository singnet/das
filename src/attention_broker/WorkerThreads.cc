#include <cstdlib>

#include "AttentionBrokerServer.h"
#include "attention_broker.grpc.pb.h"
#include "Utils.h"
#include "RequestSelector.h"
#include "WorkerThreads.h"
#include "HebbianNetworkUpdater.h"
#include "StimulusSpreader.h"

using namespace attention_broker_server;
using namespace std;

// --------------------------------------------------------------------------------
// Public methods

WorkerThreads::WorkerThreads(SharedQueue *stimulus, SharedQueue *correlation) {
    stimulus_requests = stimulus;
    correlation_requests = correlation;
    threads_count = AttentionBrokerServer::WORKER_THREADS_COUNT;
    for (unsigned int i = 0; i < threads_count; i++) {
        threads.push_back(new thread(
            &WorkerThreads::worker_thread, 
            this, 
            i,
            stimulus_requests,
            correlation_requests));
    }
}

WorkerThreads::~WorkerThreads() {
}

void WorkerThreads::graceful_stop() {
    stop_flag_mutex.lock();
    stop_flag = true;
    stop_flag_mutex.unlock();
    for (thread *worker_thread: threads) {
        worker_thread->join();
    }
}
  
// --------------------------------------------------------------------------------
// Private methods

void WorkerThreads::worker_thread(
    unsigned int thread_id, 
    SharedQueue *stimulus_requests, 
    SharedQueue *correlation_requests) {

    RequestSelector *selector = RequestSelector::factory(
            SelectorType::EVEN_THREAD_COUNT,
            thread_id,
            stimulus_requests,
            correlation_requests);
    HebbianNetworkUpdater *updater = HebbianNetworkUpdater::factory(HebbianNetworkUpdaterType::EXACT_COUNT);
    StimulusSpreader *stimulus_spreader = StimulusSpreader::factory(StimulusSpreaderType::TOKEN);
    pair<RequestType, void *> request;
    bool stop = false;
    while (! stop) {
        request = selector->next();
        if (request.second != NULL) {
            switch (request.first) {
                case RequestType::STIMULUS: {
                    stimulus_spreader->spread_stimuli((dasproto::HandleCount *) request.second);
                    break;
                }
                case RequestType::CORRELATION: {
                    updater->correlation((dasproto::HandleList *) request.second);
                    break;
                }
                default: {
                    Utils::error("Invalid request type: " + to_string((int) request.first));
                }
            }
        } else {
            this_thread::sleep_for(chrono::milliseconds(100));
            stop_flag_mutex.lock();
            if (stop_flag) {
                stop = true;
            }
            stop_flag_mutex.unlock();
        }
    }
    delete selector;
}
