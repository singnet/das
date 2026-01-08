/**
 * @file InferenceContext.h
 * @brief InferenceContext class holds context information for inference requests.
 */

#pragma once
#include <string>

using namespace std;
namespace inference_agent {

class InferenceContext {
   public:
    InferenceContext(const string& name,
                     const string& query,
                     const string& determiner_schema,
                     const string& stimulus_schema);
    ~InferenceContext();

    string get_context_name() const;

   private:
    string context_name;
};

}  // namespace inference_agent