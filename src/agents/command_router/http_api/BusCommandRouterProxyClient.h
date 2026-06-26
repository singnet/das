#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "BusCommandRouterProxy.h"

using namespace std;

namespace command_router {

/**
 * Poll a BusCommandRouterProxy for HTTP streaming consumers.
 *
 * Emits at most @p items_per_chunk answers per on_chunk callback so clients can
 * iterate results incrementally (1-by-1 or N-by-N). Terminal bus_client does
 * not use this; it keeps its own polling loop and query-engine limits.
 *
 * @param items_per_chunk Minimum 1. Each callback receives at most this many items.
 * @return true on success, false on error or abort.
 */
bool poll_router_proxy_stream(const shared_ptr<BusCommandRouterProxy>& router_proxy,
                              const string& command_type,
                              size_t items_per_chunk,
                              const function<bool()>& should_abort,
                              const function<void(const vector<string>& chunk)>& on_chunk,
                              const function<void(const string& error)>& on_error,
                              const function<void()>& on_aborted);

}  // namespace command_router
