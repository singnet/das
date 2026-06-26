#include "BusCommandRouterProxyStreamPoller.h"

#include "BaseQueryProxy.h"
#include "Utils.h"

using namespace command_router;
using namespace commons;
using namespace agents;

namespace {

void emit_chunk(const function<void(const vector<string>& chunk)>& on_chunk,
                vector<string>& chunk_data) {
    if (!chunk_data.empty() && on_chunk) {
        on_chunk(chunk_data);
        chunk_data.clear();
    }
}

void append_answer_chunk(const shared_ptr<BusCommandRouterProxy>& router_proxy,
                         bool use_metta_as_query_tokens,
                         size_t items_per_chunk,
                         const function<void(const vector<string>& chunk)>& on_chunk,
                         vector<string>& chunk_data) {
    shared_ptr<QueryAnswer> answer;
    while ((answer = router_proxy->pop()) != nullptr) {
        chunk_data.push_back(answer->to_string(use_metta_as_query_tokens));
        if (chunk_data.size() >= items_per_chunk) {
            emit_chunk(on_chunk, chunk_data);
        }
    }
    emit_chunk(on_chunk, chunk_data);
}

}  // namespace

bool BusCommandRouterProxyStreamPoller::poll_stream(
    const shared_ptr<BusCommandRouterProxy>& router_proxy,
    const string& command_type,
    size_t items_per_chunk,
    const function<bool()>& should_abort,
    const function<void(const vector<string>& chunk)>& on_chunk,
    const function<void(const string& error)>& on_error,
    const function<void()>& on_aborted) {
    if (items_per_chunk == 0) {
        if (on_error) {
            on_error("items_per_chunk must be at least 1");
        }
        return false;
    }

    auto finished_or_error = [&]() { return router_proxy->finished() || router_proxy->error_flag; };

    auto handle_abort = [&]() -> bool {
        if (should_abort && should_abort()) {
            router_proxy->abort();
            if (on_aborted) {
                on_aborted();
            }
            return true;
        }
        return false;
    };

    if (command_type == "get") {
        while (router_proxy->params_response.empty() && !finished_or_error()) {
            if (handle_abort()) {
                return false;
            }
            Utils::sleep(100);
        }
        if (router_proxy->error_flag) {
            if (on_error) {
                on_error(router_proxy->error_message);
            }
            return false;
        }
        if (on_chunk) {
            on_chunk({router_proxy->params_response});
        }
        return true;
    }

    if (command_type == "set") {
        while (router_proxy->set_param_ack.empty() && !finished_or_error()) {
            if (handle_abort()) {
                return false;
            }
            Utils::sleep(100);
        }
        if (router_proxy->error_flag) {
            if (on_error) {
                on_error(router_proxy->error_message);
            }
            return false;
        }
        if (on_chunk) {
            on_chunk({router_proxy->set_param_ack});
        }
        return true;
    }

    if (command_type == "query" || command_type == "evolution") {
        while (!router_proxy->routed_flag && !finished_or_error()) {
            if (handle_abort()) {
                return false;
            }
            Utils::sleep(100);
        }
        if (router_proxy->error_flag) {
            if (on_error) {
                on_error(router_proxy->error_message);
            }
            return false;
        }

        const bool use_metta_as_query_tokens =
            router_proxy->parameters.get_or<bool>(BaseQueryProxy::USE_METTA_AS_QUERY_TOKENS, true);

        vector<string> chunk_data;
        while (!router_proxy->finished()) {
            if (handle_abort()) {
                return false;
            }

            append_answer_chunk(
                router_proxy, use_metta_as_query_tokens, items_per_chunk, on_chunk, chunk_data);
            Utils::sleep(100);
        }
        append_answer_chunk(
            router_proxy, use_metta_as_query_tokens, items_per_chunk, on_chunk, chunk_data);

        if (router_proxy->error_flag) {
            if (on_error) {
                on_error(router_proxy->error_message);
            }
            return false;
        }
        return true;
    }

    if (on_error) {
        on_error("Unknown command_type: " + command_type);
    }
    return false;
}
