#ifndef _COMMONS_UTILS_H
#define _COMMONS_UTILS_H

#include <chrono>
#include <map>
#include <string>
#include <vector>

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

class Utils {
   public:
    Utils() {}
    ~Utils() {}

    static void error(string msg, bool throw_flag = true);
    static bool flip_coin(double true_probability = 0.5);
    static void sleep(unsigned int milliseconds = 100);
    static string get_environment(string const& key);
    static map<string, string> parse_config(string const& config_path);
    static vector<string> split(string const& str, char delimiter);
    static pair<size_t, size_t> parse_ports_range(string const& str, char delimiter = ':');
    static string join(vector<string> const& tokens, char delimiter);
    static string random_string(size_t length);
    static bool is_number(const string& s);
    static int string_to_int(const string& s);
    static float string_to_float(const string& s);
    static string trim(const string& s);
    static unsigned long long get_current_time_millis();
    static string linux_command_line(const char* cmd);
    static unsigned long get_current_free_ram();   // Kbytes
    static unsigned long get_current_ram_usage();  // Kbytes
    static bool is_port_available(unsigned int port);
    static void replace_all(string& base_string, const string& from, const string& to);
    static map<string, string> parse_command_line(int argc, char* argv[], char delimiter = '=');
};

}  // namespace commons

#endif  // _COMMONS_UTILS_H
