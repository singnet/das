#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "BusCommandRouterProxy.h"

using namespace std;

namespace command_router {

struct PollStreamResult {
    bool ok = false;
    bool is_count_only = false;
    int count_only_total = 0;
};

/**
 * Polls a BusCommandRouterProxy and delivers results in HTTP-friendly chunks.
 *
 * Used by CommandRouterHttpAPI to stream command output over WebSocket. The terminal
 * bus client keeps its own inline polling loop instead of this helper.
 */
class BusCommandRouterProxyStreamPoller {
   public:
    /**
     * Poll router_proxy until the command finishes, is aborted, or fails.
     *
     * For command_type "query" and "evolution", answers are popped from the proxy
     * and forwarded in batches of at most items_per_chunk strings. For "get" and
     * "set", a single chunk is emitted once the proxy response is ready.
     *
     * @param router_proxy Proxy already issued on the service bus.
     * @param command_type Router command: "get", "set", "query", or "evolution".
     * @param items_per_chunk Maximum answers per on_chunk call for query/evolution.
     *                        Must be at least 1.
     * @param should_abort Optional callback; when it returns true, the proxy is aborted
     *                     and on_aborted is invoked.
     * @param on_chunk Called with each batch of serialized answers or response payload.
     * @param on_error Called with an error message on validation, proxy, or unknown
     *                 command failures.
     * @param on_aborted Called when polling stops because should_abort returned true.
     * @return Result status; count-only totals are returned when is_count_only is true.
     */
    static PollStreamResult poll_stream(const shared_ptr<BusCommandRouterProxy>& router_proxy,
                                        const string& command_type,
                                        size_t items_per_chunk,
                                        const function<bool()>& should_abort,
                                        const function<void(const vector<string>& chunk)>& on_chunk,
                                        const function<void(const string& error)>& on_error,
                                        const function<void()>& on_aborted);

   private:
    BusCommandRouterProxyStreamPoller() = delete;
};

}  // namespace command_router
