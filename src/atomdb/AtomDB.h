#pragma once

#include "AtomDBAPITypes.h"

using namespace std;

namespace atomdb {

class AtomDB {
   public:
    AtomDB() = default;
    virtual ~AtomDB() = default;

    static inline string WILDCARD = "*";

    virtual shared_ptr<atomdb_api_types::HandleSet> query_for_pattern(
        shared_ptr<char> pattern_handle) = 0;
    virtual shared_ptr<atomdb_api_types::HandleList> query_for_targets(shared_ptr<char> link_handle) = 0;
    virtual shared_ptr<atomdb_api_types::HandleList> query_for_targets(char* link_handle_ptr) = 0;
    virtual shared_ptr<atomdb_api_types::AtomDocument> get_atom_document(const char* handle) = 0;
    virtual bool link_exists(const char* link_handle) = 0;
    virtual std::vector<std::string> links_exist(const std::vector<std::string>& link_handles) = 0;
    virtual char* add_node(const char* type,
                           const char* name,
                           const atomdb_api_types::CustomAttributesMap& custom_attributes) = 0;
    virtual char* add_link(const char* type,
                           char** targets,
                           size_t targets_size,
                           const atomdb_api_types::CustomAttributesMap& custom_attributes) = 0;

   private:
    virtual void attention_broker_setup() = 0;
};

}  // namespace atomdb
