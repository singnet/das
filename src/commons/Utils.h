#ifndef _COMMONS_UTILS_H
#define _COMMONS_UTILS_H

#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <functional>
#include <map>
#include <mutex>
#include <stack>
#include <string>
#include <vector>

#include "Logger.h"

using namespace std;

namespace commons {

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

class MemoryFootprint {
   public:
    MemoryFootprint();
    ~MemoryFootprint();
    void start();
    void check(const string& tag = "");
    void stop(const string& tag = "");
    long delta_usage(bool since_last_check = false);
    string to_string();

   private:
    bool running;
    unsigned long start_snapshot;
    unsigned long last_snapshot;
    unsigned long final_snapshot;
    vector<pair<long, string>> delta_ram;
};

class StackTrace {
   private:
    class StackRecord {
       public:
        string file;
        string function;
        int line;
        string message;
        string to_string() {
            return file + "#L" + std::to_string(line) + " " + function + " " + message;
        }
    };
    static void push(StackRecord& record) {
        lock_guard<mutex> semaphore(api_mutex);
        pid_t tid = gettid();
        if (stack_trace.find(tid) == stack_trace.end()) {
            stack_trace[tid] = stack<StackRecord>();
        }
        stack_trace[tid].push(record);
    }
    static void pop() {
        lock_guard<mutex> semaphore(api_mutex);
        pid_t tid = gettid();
        if (stack_trace.find(tid) == stack_trace.end()) {
            LOG_ERROR("Invalid attempt to unstack a record from a non-existent stack trace");
            return;
        }
        stack_trace[tid].pop();
    }
    static mutex api_mutex;
    static map<pid_t, stack<StackRecord>> stack_trace;

   public:
    StackTrace(const string& file, const string& function, int line, const string& message) {
        StackRecord record;
        record.file = file;
        record.function = function;
        record.line = line;
        record.message = message;
        StackTrace::push(record);
    }
    ~StackTrace() { StackTrace::pop(); }
    static string to_string() {
        lock_guard<mutex> semaphore(api_mutex);
        string answer = "";
        if (stack_trace.size() > 0) {
            answer += "\n-------------------- Stack trace --------------------\n";
            for (auto pair : stack_trace) {
                answer += "Thread tid: " + std::to_string((unsigned int) pair.first) + "\n";
                stack<StackRecord> stack_copy = pair.second;
                while (!stack_copy.empty()) {
                    answer += "  " + stack_copy.top().to_string() + "\n";
                    stack_copy.pop();
                }
            }
            answer += "-----------------------------------------------------\n";
        }
        return answer;
    }
};

class Utils {
   public:
    Utils() {}
    ~Utils() {}

    static void error(string msg, bool throw_flag, bool log_flag = true);
    static bool flip_coin(double true_probability = 0.5);
    static unsigned int uint_rand(unsigned int open_upper_bound);
    static unsigned int uint_rand(unsigned int closed_lower_bound, unsigned int open_upper_bound);
    static void sleep(unsigned int milliseconds = 100);
    static string get_environment(string const& key);
    static map<string, string> parse_config(string const& config_path);
    static vector<string> split(string const& str, char delimiter);
    static pair<size_t, size_t> parse_ports_range(string const& str, char delimiter = ':');
    static string join(vector<string> const& tokens, char delimiter = ' ');
    static string random_string(size_t length);
    static string random_string(size_t length, const string& charset);
    static bool is_number(const string& s);
    static int string_to_int(const string& s);
    static unsigned int string_to_uint(const string& s);
    static float string_to_float(const string& s);
    static string trim(const string& s);
    static unsigned long long get_current_time_millis();
    static string linux_command_line(const char* cmd);
    static unsigned long get_current_free_ram();   // Kbytes
    static unsigned long get_total_ram();          // Kbytes
    static unsigned long get_current_ram_usage();  // Kbytes
    static bool is_port_available(unsigned int port);
    static void replace_all(string& base_string, const string& from, const string& to);
    static map<string, string> parse_command_line(int argc, char* argv[], char delimiter = '=');
    static void retry_function(function<void()> func,
                               unsigned int max_retries,
                               unsigned int wait_millis,
                               const string& function_name = "");
    static bool read_and_split(vector<string>& output, ifstream& file, char delimiter = ' ');

    template <class C>
    static bool intersects(const C& set1, const C& set2) {
        auto iterator1 = set1.begin();
        auto iterator2 = set2.begin();
        while (iterator1 != set1.end() && iterator2 != set2.end()) {
            if (*iterator1 < *iterator2) {
                iterator1++;
            } else if (*iterator2 < *iterator1) {
                iterator2++;
            } else {
                return true;
            }
        }
        return false;
    }
};

}  // namespace commons

#define RAISE_ERROR(msg)                                  \
    {                                                     \
        LOG_ERROR(string(msg) + StackTrace::to_string()); \
        Utils::error(msg, true, false);                   \
    }

#ifdef LOG_LEVEL
#if LOG_LEVEL >= DEBUG_LEVEL
#define STACK_TRACE(msg) StackTrace scope_variable(__FILE__, __FUNCTION__, __LINE__, msg);
#else
#define STACK_TRACE(msg)
#endif
#endif
#endif  // _COMMONS_UTILS_H
