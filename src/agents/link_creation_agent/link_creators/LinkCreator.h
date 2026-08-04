#pragma once

#include <set>
#include <string>

#include "QueryAnswer.h"

using namespace std;
using namespace query_engine;

namespace link_creators {

/**
 * Processes a QueryAnswer and creates one or more links using any of the elements in it.
 * LinkCreator is NOT a thread-safe object.
 */
class LinkCreator {
   public:
    LinkCreator();
    ~LinkCreator();

    /**
     * Create or update one or more links using the passed QueryAnswer. The actual number of links
     * <created, updated> is returned in a pair.
     *
     * @param query_answer The QueryAnswer object used to build lin(s).
     * @return A pair <CREATED, UPDATED> with the actual number of links which have been
     * created or updated.
     */
    virtual pair<unsigned int, unsigned int> create(shared_ptr<QueryAnswer> query_answer) = 0;

    /**
     * Return the AttentionBroker context to be used.
     *
     * @return the AttentionBroker context to be used.
     */
    inline const string& context() { return this->_context; }

    /**
     * Setter for the context to be used in AttentionBroker.
     *
     * @param context The AttentionBroker context.
     */
    inline void set_context(const string& context) { this->_context = context; }

    /**
     * Get the boolean value of the flag _visited_at_least_one_in_last_create and reset it to false.
     * This flag indicates wether some key have been visited since the last time this method was
     * called. The idea is to call it once just after calling create() so it can check if this
     * last execution of create() has visited any new key.
     *
     * Visited keys are keep to avoid the attempt to create links for parameters (obtained from
     * QueryAnswer objects) that have already been seen. This is because the mere fact of trying
     * to create these links can be an expensive operation so we don't want to waste time
     * creating links just to discover later on that they have already been inserted into the
     * AtomDB.
     */
    bool get_and_reset_visited();

   protected:
    void visit(const string& key);
    bool visited(const string& key);

   private:
    bool _visited_at_least_one_in_last_create;
    set<string> _visited;
    string _context;
};

}  // namespace link_creators
