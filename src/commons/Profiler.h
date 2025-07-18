#include "Utils.h"

#if LOG_LEVEL >= DEBUG_LEVEL
#define STOP_WATCH_START(var_prefix)         \
    StopWatch var_prefix##StopWatchProfiler; \
    var_prefix##StopWatchProfiler.start();
#else
#define STOP_WATCH_START(var_prefix)
#endif

#if LOG_LEVEL >= DEBUG_LEVEL
#define STOP_WATCH_STOP(var_prefix) var_prefix##StopWatchProfiler.stop();
#else
#define STOP_WATCH_STOP(var_prefix) var_prefix##StopWatchProfiler.stop();
#endif

#if LOG_LEVEL >= DEBUG_LEVEL
#define STOP_WATCH_RESTART(var_prefix) var_prefix##StopWatchProfiler.start();
#else
#define STOP_WATCH_RESTART(var_prefix) var_prefix##StopWatchProfiler.start();
#endif

#if LOG_LEVEL >= DEBUG_LEVEL
#define STOP_WATCH_FINISH(var_prefix, tag)                                                \
    var_prefix##StopWatchProfiler.stop();                                                 \
    LOG_DEBUG("#PROFILER# STOP_WATCH \"" + string(tag) + "\" " +                          \
              std::to_string(var_prefix##StopWatchProfiler.milliseconds()) + " " + "\"" + \
              var_prefix##StopWatchProfiler.str_time() + "\"");                           \
    var_prefix##StopWatchProfiler.reset();
#else
#define STOP_WATCH_FINISH(var_prefix, tag)
#endif
