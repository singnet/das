#ifndef _ATTENTION_BROKER_SERVER_STIMULUSSPREADER_H
#define _ATTENTION_BROKER_SERVER_STIMULUSSPREADER_H

#include "attention_broker.grpc.pb.h"
#include "Utils.h"
#include "HebbianNetwork.h"

using namespace std;

namespace attention_broker_server {

/**
 * Algorithm used to update HebbianNetwork weights in "stimulate" requests.
 */
enum class StimulusSpreaderType {
    TOKEN /// Consider importance as a fixed amount of tokens distributed among atoms in the HebbianNetwork.
};

/**
 * Process stimuli spreading requests by boosting importance of the atoms passed in the request
 * and running one cycle of stimuli spreading in the Hebbian Network.
 *
 * Objects of this class are used by worker threads to process "stimulate" requests.
 *
 * The request have a list of pairs (handle, n) with the handles whose importance should be
 * boosted and the relative magnitude of this boost (compared to the other handles in the same
 * request).
 *
 * This is an abstract class. Concrete subclasses implement different ways of spreading stimuli
 * in the HebbianNetwork.
 * 
 */
class StimulusSpreader {

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
    static StimulusSpreader *factory(StimulusSpreaderType instance_type);
    virtual ~StimulusSpreader(); /// destructor.

    /**
     * Stimulate atoms and run one cycle of stimuli spreading.
     *
     * Atoms in the passed list have their importance boosted according to the passed counts. Then
     * one cycle of stimuli spreading is executed.
     *
     * @param request A list of handles to be boosted and respective counts which are used to determine
     * the magnitude of such boost. The actual way importance is boosted and then spread among HebbianNetwork
     * links are delegated to the concrete subclasses.
     */
    virtual void spread_stimuli(const dasproto::HandleCount *request) = 0;

protected:

    StimulusSpreader(); /// Basic empty constructor.

private:

};

/**
 * Process stimuli spreading requests by boosting importance of the atoms passed in the request
 * and running one cycle of stimuli spreading in the Hebbian Network.
 *
 * Objects of this class are used by worker threads to process "stimulate" requests.
 *
 * The request have a list of pairs (handle, n) with the handles whose importance should be
 * boosted and the relative magnitude of this boost (compared to the other handles in the same
 * request).
 *
 * This StimulusSpreader consider a fixed amount of tokens distributed among all atoms in the
 * HebbianNetwork. Importance boosts and stimulus spreading are implemented in a way that this
 * total amount of tokens remains fixed, unless explicitly requested by caller.
 */
class TokenSpreader: public StimulusSpreader {

public:

    TokenSpreader(); /// Basic empty constructor.
    ~TokenSpreader(); /// Destructor.

    // data structure used as parameter container in "visit" functions
    // used in trie traversal
    typedef struct {
        ImportanceType rent_rate;
        ImportanceType total_rent;
        HandleTrie *importance_changes;
        unsigned int largest_arity;
        ImportanceType spreading_rate_lowerbound;
        ImportanceType spreading_rate_range_size;
        ImportanceType to_spread;
        double sum_weights;
    } StimuliData;

    // data structure used in a private trie during importance update calculations
    class ImportanceChanges: public HandleTrie::TrieValue {
        public:
            ImportanceType rent;
            ImportanceType wages;
            ImportanceChanges(ImportanceType r, ImportanceType w) {
                rent = r;
                wages = w;
            }
            void merge(TrieValue *other) {
                rent += ((ImportanceChanges *) other)->rent;
                wages += ((ImportanceChanges *) other)->wages;
            }
    };

    /**
     * Stimulate atoms and run one cycle of stimuli spreading.
     *
     * Atoms in the passed list have their importance boosted according to the passed counts. Then
     * one cycle of stimuli spreading is executed.
     *
     * Boosts and stimuli spreading are actually tokens which are collected from all the nodes in the 
     * HebbianNetwork (as a rent) and redistributed according to the passed * counts (as wages). Once 
     * rents and wages are consolidated in each node's importance, one cycle of stimuli spreading is run 
     * when  a % of the importance tokens of each node being redistributed to amnongst its neighbors 
     * according to the weights of the links in the HebbianNetwork.
     *
     * @param request A list of handles to be boosted and respective counts which are used to determine
     * the magnitude of such boost.
     */
    void spread_stimuli(const dasproto::HandleCount *request);

    // Used only in "visit" functions during trie traversals. Such functions aren't methods so this method
    // must be public.
    void distribute_wages(const dasproto::HandleCount *handle_count, ImportanceType &total_to_spread, StimuliData *data);

};

} // namespace attention_broker_server

#endif // _ATTENTION_BROKER_SERVER_STIMULUSSPREADER_H
