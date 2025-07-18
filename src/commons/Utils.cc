#include "Utils.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <unistd.h>
#include <ios>
#include <string>

#include "Logger.h"

using namespace commons;
using namespace std;

// --------------------------------------------------------------------------------
// Public methods

Utils::Utils() {}

Utils::~Utils() {}

void Utils::error(string msg) {
    LOG_ERROR(msg);
    throw runtime_error(msg);
}

void Utils::warning(string msg) { cerr << msg << endl; }

bool Utils::flip_coin(double true_probability) {
    long f = 1000;
    return (rand() % f) < lround(true_probability * f);
}

void Utils::sleep(unsigned int milliseconds) {
    this_thread::sleep_for(chrono::milliseconds(milliseconds));
}

string Utils::get_environment(string const& key) {
    char* value = getenv(key.c_str());
    string answer = (value == NULL ? "" : value);
    return answer;
}

StopWatch::StopWatch() { reset(); }

StopWatch::~StopWatch() {}

void StopWatch::start() {
    if (running) {
        stop();
    }
    start_time = chrono::steady_clock::now();
    running = true;
}

void StopWatch::stop() {
    if (running) {
        chrono::steady_clock::time_point check = chrono::steady_clock::now();
        accumulator = accumulator + (check - start_time);
        start_time = check;
        running = false;
    }
}

void StopWatch::reset() {
    running = false;
    accumulator = chrono::steady_clock::duration::zero();
}

unsigned long StopWatch::milliseconds() {
    return chrono::duration_cast<chrono::milliseconds>(accumulator).count();
}

string StopWatch::str_time() {
    unsigned long millis = milliseconds();

    unsigned long seconds = millis / 1000;
    if (seconds > 0) {
        millis = millis % 1000;
    }

    unsigned long minutes = seconds / 60;
    if (minutes > 0) {
        seconds = seconds % 60;
    }

    unsigned long hours = minutes / 60;
    if (hours > 0) {
        minutes = minutes % 60;
    }

    if (hours > 0) {
        return to_string(hours) + " hours " + to_string(minutes) + " mins";
    } else if (minutes > 0) {
        return to_string(minutes) + " mins " + to_string(seconds) + " secs";
    } else {
        return to_string(seconds) + " secs " + to_string(millis) + " millis";
    }
}

map<string, string> Utils::parse_config(string const& config_path) {
    map<string, string> config;
    ifstream file(config_path);
    string line;
    while (getline(file, line)) {
        istringstream iss_line(line);
        string key;
        if (getline(iss_line, key, '=')) {
            string value;
            if (getline(iss_line, value)) {
                value.erase(remove_if(value.begin(), value.end(), ::isspace), value.end());
                key.erase(remove_if(key.begin(), key.end(), ::isspace), key.end());
                config[key] = value;
            }
        }
    }
    file.close();
    return config;
}

vector<string> Utils::split(const string& s, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

pair<size_t, size_t> Utils::parse_ports_range(const string& str, char delimiter) {
    auto tokens = split(str, delimiter);
    if (tokens.size() != 2 || !is_number(tokens[0]) || !is_number(tokens[1])) {
        throw invalid_argument("Invalid port range format. Use <start_port:end_port>.");
    }
    size_t start_port = stoul(tokens[0]);
    size_t end_port = stoul(tokens[1]);
    if (start_port >= end_port) {
        throw invalid_argument("Invalid port range: start port must be less than end port.");
    }
    return make_pair(start_port, end_port);
}

string Utils::join(const vector<string>& tokens, char delimiter) {
    string result;
    for (size_t i = 0; i < tokens.size(); i++) {
        if (i > 0) {
            result += delimiter;
        }
        result += tokens[i];
    }
    return result;
}

string Utils::random_string(size_t length) {
    const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    const size_t max_index = (sizeof(charset) - 1);
    string result;
    for (size_t i = 0; i < length; i++) {
        result += charset[rand() % max_index];
    }
    return result;
}

bool Utils::is_number(const string& s) {
    return !s.empty() &&
           find_if(s.begin(), s.end(), [](unsigned char c) { return !isdigit(c); }) == s.end();
}

int Utils::string_to_int(const string& s) {
    if (!is_number(s)) {
        throw invalid_argument("Can not convert string to int: Invalid arguments");
    }
    return stoi(s);
}

string Utils::trim(const string& s) {
    const string whitespace = " \n\r\t\f\v";
    size_t start = s.find_first_not_of(whitespace);
    if (start == string::npos) {
        return "";
    }
    size_t end = s.find_last_not_of(whitespace);
    return s.substr(start, end - start + 1);
}

unsigned long long Utils::get_current_time_millis() {
    return chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now().time_since_epoch())
        .count();
}

string Utils::linux_command_line(const char* cmd) {
    std::array<char, 128> buffer;
    std::string answer;
    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        Utils::error("Command line failed");
        return "";
    }
    try {
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            answer += buffer.data();
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return answer;
}

unsigned long Utils::get_current_free_ram() {
    return std::stol(Utils::linux_command_line("cat /proc/meminfo | grep MemAvailable | rev | cut -d\" \" -f2 | rev"));
}

unsigned long Utils::get_current_ram_usage() {
   using std::ios_base;
   using std::ifstream;
   using std::string;

   //double vm_usage = 0.0;
   double resident_set = 0.0;

   ifstream stat_stream("/proc/self/stat", ios_base::in);

   // Not actually used
   string pid, comm, state, ppid, pgrp, session, tty_nr;
   string tpgid, flags, minflt, cminflt, majflt, cmajflt;
   string utime, stime, cutime, cstime, priority, nice;
   string O, itrealvalue, starttime;

   unsigned long vsize;
   long rss;

   stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
               >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
               >> utime >> stime >> cutime >> cstime >> priority >> nice
               >> O >> itrealvalue >> starttime >> vsize >> rss;

   stat_stream.close();

   long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024;
   //vm_usage = vsize / 1024.0;
   resident_set = rss * page_size_kb;

   return (unsigned long) resident_set;
}
