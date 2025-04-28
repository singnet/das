#pragma once

#include <memory>

#include "QueryElement.h"
#include "attention_broker.pb.h"

using namespace std;

namespace query_element {
/**
 * @brief A QueryElement that stores an ImportanceList.
 *
 * @details This class is the result of a very specific requirement: we need to
 * store ImportanceList objects in a cache-like structure, like the
 * QueryElementRegistry. The QueryElementRegistry is a cache that stores
 * QueryElements and retrieves them based on their key. The ImportanceList is
 * not a QueryElement, but we want to store it in the QueryElementRegistry.
 * To solve this, we created this class that wraps an ImportanceList and makes
 * it a QueryElement.
 */
class ImportanceList : public QueryElement {
   public:
    /**
     * @brief Constructor.
     *
     * @param importance_list The ImportanceList to be stored.
     */
    ImportanceList(shared_ptr<dasproto::ImportanceList> importance_list) : QueryElement() {
        this->importance_list = importance_list;
    }

    ~ImportanceList() override {}

    /**
     * @brief The ImportanceList stored in this object.
     */
    shared_ptr<dasproto::ImportanceList> importance_list;

   private:
    void setup_buffers() override {}

    void graceful_shutdown() override {}
};
}  // namespace query_element
