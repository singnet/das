#pragma once

#include "Utils.h"

using namespace std;

namespace mains {

enum class ProcessorType {
    INFERENCE_AGENT,
    LINK_CREATION_AGENT,
    CONTEXT_BROKER,
    EVOLUTION_AGENT,
    QUERY_ENGINE,
    UNKNOWN
};

enum class ServiceCallerType { CLIENT, NODE, UNKNOWN };

class RunnerHelper {
   public:
    RunnerHelper() = default;
    ~RunnerHelper() = default;

    static bool is_running;

    static string help(const ProcessorType& processor_type,
                       ServiceCallerType caller_type = ServiceCallerType::UNKNOWN);

    static vector<string> get_required_arguments(
        const string& processor_type, ServiceCallerType caller_type = ServiceCallerType::UNKNOWN);

    static ProcessorType processor_type_from_string(const string& type_str);
};
}  // namespace mains