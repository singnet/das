#ifndef _QUERY_ELEMENT_REMOTESINK_H
#define _QUERY_ELEMENT_REMOTESINK_H

#include "Sink.h"
#include "SharedQueue.h"
//#include "HandleTrie.h"

using namespace std;
//using namespace attention_broker_server;

namespace query_element {

/**
 * A special sink which forwards the query results to a remote QueryElement (e.g. a RemoteIterator).
 */
class RemoteSink: public Sink {

public:

    /**
     * Constructor.
     *
     * @param precedent QueryElement just below in the query tree.
     * @param local_id ID of this element in the network connecting to the remote
     *        peer (typically "host:port").
     * @param remote_id network ID of the remote peer (typically "host:port").
     * @param delete_precedent_on_destructor If true, the destructor of this QueryElement will
     *        also destruct the passed precedent QueryElement (defaulted to false).
     */
    RemoteSink(
        QueryElement *precedent, 
        const string &local_id,
        const string &remote_id,
        bool update_attention_broker_flag = false,
        const string &context = "",
        bool delete_precedent_on_destructor = false);

    /**
     * Destructor.
     */
    ~RemoteSink();

    // --------------------------------------------------------------------------------------------
    // QueryElement API

    /**
     * Setups QueryNode elements related to the communication with the remote element.
     */
    virtual void setup_buffers();

    /**
     * Gracefully shuts down the queue processor thread and the remote communication QueryNodes
     * present in this QueryElement.
     */
    virtual void graceful_shutdown();

    /**
     * Sources tipically need to communicate with the AttentionBroker in order to sort links
     * by importance. AttentionBroker is supposed to be running in the same machine as all
     * Source elements so only a port number is required. Here we provide a default value
     * in the case none is passed in constructor.
     */
    static string DEFAULT_ATTENTION_BROKER_PORT;

private:

    shared_ptr<QueryNode> remote_output_buffer;
    string local_id;
    string remote_id;
    thread *queue_processor;
    thread *attention_broker_postprocess;
    SharedQueue attention_broker_queue;
    bool attention_broker_postprocess_finished;
    mutex attention_broker_postprocess_finished_mutex;
    string attention_broker_address;
    bool update_attention_broker_flag;
    string query_context;

    void queue_processor_method();
    void attention_broker_postprocess_method();
    bool is_attention_broker_postprocess_finished();
    void set_attention_broker_postprocess_finished();
    //void update_attention_broker(QueryAnswer *query_answer);

};

/*
class AccumulatorValue: public HandleTrie::TrieValue {
public:
    unsigned int count;
    AccumulatorValue() {
        this->count = 1;
    }
    void merge(TrieValue *other) {
        count += ((AccumulatorValue *) other)->count;
    }
};
*/

} // namespace query_element

#endif // _QUERY_ELEMENT_REMOTESINK_H
