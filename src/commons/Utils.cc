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

using namespace commons;
using namespace std;

// --------------------------------------------------------------------------------
// Public methods

Utils::Utils() {}

Utils::~Utils() {}

void Utils::error(string msg) { throw runtime_error(msg); }
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
    millis = millis % 60;

    unsigned long minutes = seconds / 60;
    seconds = seconds % 60;

    unsigned long hours = minutes / 60;
    minutes = minutes % 60;

    if (hours > 0) {
        return to_string(hours) + " hours " + to_string(minutes) + " mins";
    } else if (minutes > 0) {
        return to_string(minutes) + " mins " + to_string(seconds) + " secs";
    } else if (seconds > 0) {
        // double s = ((double) ((seconds * 1000) + millis)) / 1000.0;
        // stringstream stream;
        // stream << fixed << setprecision(3) << s;
        // return stream.str() + " secs";
        return to_string(seconds) + " secs " + to_string(millis) + " millis";
    } else {
        return to_string(millis) + " millis";
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
// TODO move this to a utils file
int Utils::string_to_int(const string& s) {
    if (!is_number(s)) {
        throw invalid_argument("Can not convert string to int: Invalid arguments");
    }
    return stoi(s);
}
