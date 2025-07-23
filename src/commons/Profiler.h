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
#define STOP_WATCH_STOP(var_prefix)
#endif

#if LOG_LEVEL >= DEBUG_LEVEL
#define STOP_WATCH_RESTART(var_prefix) var_prefix##StopWatchProfiler.start();
#else
#define STOP_WATCH_RESTART(var_prefix)
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

#if LOG_LEVEL >= DEBUG_LEVEL
#define RAM_CHECKPOINT(tag)                                          \
    LOG_DEBUG("#PROFILER# RAM_CHECKPOINT \"" + string(tag) + "\" " + \
              std::to_string(Utils::get_current_free_ram()) + " Kbytes available");
#else
#define RAM_CHECKPOINT(tag)
#endif

#if LOG_LEVEL >= DEBUG_LEVEL
#define RAM_FOOTPRINT_START(var_prefix)                  \
    MemoryFootprint var_prefix##MemoryFootprintProfiler; \
    var_prefix##MemoryFootprintProfiler.start();
#else
#define RAM_FOOTPRINT_START(var_prefix)
#endif

#if LOG_LEVEL >= DEBUG_LEVEL
#define RAM_FOOTPRINT_CHECK(var_prefix, tag) var_prefix##MemoryFootprintProfiler.check(tag);
#else
#define RAM_FOOTPRINT_CHECK(var_prefix, tag)
#endif

#if LOG_LEVEL >= DEBUG_LEVEL
#define RAM_FOOTPRINT_FINISH(var_prefix, tag)      \
    var_prefix##MemoryFootprintProfiler.stop(tag); \
    LOG_DEBUG("#PROFILER# RAM_FOOTPRINT " + var_prefix##MemoryFootprintProfiler.to_string());
#else
#define RAM_FOOTPRINT_FINISH(var_prefix, tag)
#endif
