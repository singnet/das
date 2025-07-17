#include "Utils.h"

#define \
    STOP_WATCH_START(var_prefix)                 \
        StopWatch var_prefix##StopWatchProfiler; \
        var_prefix##StopWatchProfiler.start();

#define \
    STOP_WATCH_STOP(var_prefix, tag)             \
        var_prefix##StopWatchProfiler.stop();

#define \
    STOP_WATCH_FINISH(var_prefix, tag)                         \
        var_prefix##StopWatchProfiler.stop();                  \
        LOG_DEBUG("#PROFILER# STOP_WATCH <" +                  \
            string(tag) + "> " +                                       \
            std::to_string(var_prefix##StopWatchProfiler.milliseconds()) + " " + \
            "\"" + var_prefix##StopWatchProfiler.str_time() + "\"");         \
        var_prefix##StopWatchProfiler.reset();
