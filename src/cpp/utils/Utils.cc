#include <cstdlib>
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>

#include "Utils.h"

using namespace commons;
using namespace std;

// --------------------------------------------------------------------------------
// Public methods

Utils::Utils() {
}

Utils::~Utils() {
}

void Utils::error(string msg) {
    throw runtime_error(msg);
}
void Utils::warning(string msg) {
    cerr << msg << endl;
}

bool Utils::flip_coin(double true_probability) {
    long f = 1000;
    return (rand() % f) < lround(true_probability * f);
}

void Utils::sleep(unsigned int milliseconds) {
    this_thread::sleep_for(chrono::milliseconds(milliseconds));
}

string Utils::get_environment(string const &key) {
    char *value = getenv(key.c_str());
    string answer = (value == NULL ? "" : value);
    return answer;
}

StopWatch::StopWatch() {
    reset();
}

StopWatch::~StopWatch() {
}

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
    return std::chrono::duration_cast<std::chrono::milliseconds>(accumulator).count();
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
        //double s = ((double) ((seconds * 1000) + millis)) / 1000.0;
        //std::stringstream stream;
        //stream << std::fixed << std::setprecision(3) << s;
        //return stream.str() + " secs";
        return to_string(seconds) + " secs " + to_string(millis) + " millis";
    } else {
        return to_string(millis) + " millis";
    }
}
