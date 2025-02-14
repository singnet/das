#ifndef _ATTENTION_BROKER_SERVER_HEBBIANNETWORKUPDATER_H
#define _ATTENTION_BROKER_SERVER_HEBBIANNETWORKUPDATER_H

#include "attention_broker.grpc.pb.h"

using namespace std;

namespace attention_broker_server {

/**
 * Algorithm used to update HebbianNetwork weights in "correlate" requests.
 */
enum class HebbianNetworkUpdaterType {
    EXACT_COUNT /// Tracks counts of nodes and links computing actual weights on demand.
};

/**
 * Process correlation requests by changing the weights in the passed HebbianNetwork
 * to reflect the evidence provided in the request.
 *
 * Objects of this class are used by worker threads to process "correlation" requests.
 *
 * The request have a list of handles of atoms which appeared together in the same query
 * answer. So it's a positive evidence of correlation among such atoms. The HebbianNetwork
 * weights are changed to reflect this evidence.
 *
 * This is an abstract class. Concrete subclasses implement different ways of computing
 * weights in HebbianNetwork.
 */
class HebbianNetworkUpdater {

public:

    /**
     * Factory method.
     *
     * Factory method to instantiate concrete subclasses according to the passed parameter.
     *
     * @param instance_type Type of concrete subclass to be instantiated.
     *
     * @return An object of the passed type.
     */
    static HebbianNetworkUpdater *factory(HebbianNetworkUpdaterType instance_type);
    virtual ~HebbianNetworkUpdater(); /// Destructor.

    /**
     * Process a correlation evidence.
     *
     * The evidence is used to update the weights in the HebbianNetwork. The actual way these
     * weights are updated depends on the type of the concrete subclass that implements this method.
     *
     * @param request A list of handles of atoms which appeared in the same query answer.
     */
    virtual void correlation(const dasproto::HandleList* request) = 0;

protected:

    HebbianNetworkUpdater(); /// Basic empty constructor.

private:

};

/**
 * Process correlation requests by changing the weights in the passed HebbianNetwork
 * to reflect the evidence provided in the request.
 *
 * Objects of this class are used by worker threads to process "correlation" requests.
 *
 * The request have a list of handles of atoms which appeared together in the same query
 * answer. So it's a positive evidence of correlation among such atoms. The HebbianNetwork
 * weights are changed to reflect this evidence.
 *
 * This HebbianNetworkUpdater keeps track of actual counts of atoms and symmetric hebbian
 * links between them as they appear in correlation evidence (requests). Actual hebbian weights
 * between A -> B are calculated on demand by dividing count(A->B) / count(A).
 */
class ExactCountHebbianUpdater: public HebbianNetworkUpdater {

public:

    ExactCountHebbianUpdater(); /// Basic empty constructor.
    ~ExactCountHebbianUpdater(); /// Destructor.

    /**
     * Process a correlation evidence.
     *
     * The evidence is used to update the weights in the HebbianNetwork. 
     *
     * This HebbianNetworkUpdater keeps track of actual counts of atoms and symmetric hebbian
     * links between them as they appear in correlation evidence (requests). Actual hebbian weights
     * between A -> B are calculated on demand by dividing count(A->B) / count(A).
     *
     * @param request A list of handles of atoms which appeared in the same query answer.
     */
    void correlation(const dasproto::HandleList *request); /// Process a correlation evidence.
};

} // namespace attention_broker_server

#endif // _ATTENTION_BROKER_SERVER_HEBBIANNETWORKUPDATER_H
