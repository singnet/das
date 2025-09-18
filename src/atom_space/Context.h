#pragma once

#include <string>
#include <vector>

#include "Atom.h"
#include "QueryAnswer.h"

#define CACHE_FILE_NAME_PREFIX "_CONTEXT_CACHE_"

using namespace std;
using namespace atoms;
using namespace query_engine;

namespace atom_space {

/**
 *
 */
class Context {
    friend class AtomSpace;

   public:
    ~Context();
    void add_determiners(const vector<string>& query,
                         const vector<pair<QueryAnswerElement, QueryAnswerElement>>& determiner_schema,
                         vector<QueryAnswerElement>& stimulus_schema);
    const string& get_key();
    const string& get_name();

   protected:
    Context(const string& name, Atom& atom_key);
    Context(const string& name,
            const vector<string>& query,
            const vector<pair<QueryAnswerElement, QueryAnswerElement>>& determiner_schema,
            vector<QueryAnswerElement>& stimulus_schema);

   private:
    void init(const string& name);
    void cache_write();
    bool cache_read();
    void update_attention_broker();
    map<string, unsigned int> to_stimulate;
    vector<vector<string>> determiner_request;
    bool cached;
    string cache_file_name;
    string name;
    string key;
};

}  // namespace atom_space
