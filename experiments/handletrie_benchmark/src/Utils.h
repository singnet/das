#ifndef _ATTENTION_BROKER_SERVER_UTILS_H
#define _ATTENTION_BROKER_SERVER_UTILS_H

#include <string>
#include <chrono>

using namespace std;

namespace attention_broker_server {

class StopWatch {
    public:
        StopWatch();
        ~StopWatch();
        void start();
        void stop();
        void reset();
        unsigned long milliseconds();
        string str_time();
    private:
        bool running;
        chrono::steady_clock::time_point start_time;
        chrono::steady_clock::duration accumulator;
};

class Utils {

public:

    Utils();
    ~Utils();

    static void error(string msg);
    static void warning(string msg);
    static bool flip_coin(double true_probability = 0.5);
};

} // namespace attention_broker_server

#endif // _ATTENTION_BROKER_SERVER_UTILS_H
