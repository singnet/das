#include "Utils.h"
#include "gtest/gtest.h"

#define LOG_LEVEL DEBUG_LEVEL
#include "Logger.h"

using namespace commons;
using namespace std;

TEST(Profiling, basics) {
    MemoryFootprint footprint;

    STOP_WATCH_START(test);

    RAM_CHECKPOINT("RAM_begin");
    RAM_FOOTPRINT_START(fp);
    EXPECT_THROW(footprint.check(), runtime_error);
    EXPECT_THROW(footprint.stop(), runtime_error);
    EXPECT_EQ(footprint.delta_usage(true), 0L);
    EXPECT_EQ(footprint.delta_usage(false), 0L);
    footprint.start();
    cout << footprint.to_string() << endl;
    footprint.check("A");
    cout << footprint.to_string() << endl;
    footprint.check("B");
    cout << footprint.to_string() << endl;
    footprint.check("");
    cout << footprint.to_string() << endl;
    footprint.stop("C");
    cout << footprint.to_string() << endl;
    cout << footprint.delta_usage() << endl;
    RAM_CHECKPOINT("RAM_end");
    RAM_FOOTPRINT_CHECK(fp, "1");

    STOP_WATCH_STOP(test);
    STOP_WATCH_RESTART(test);
    STOP_WATCH_STOP(test);
    RAM_FOOTPRINT_FINISH(fp, "2");
    STOP_WATCH_FINISH(test, "TEST END");
}
