#include "ConfigFileSingleton.h"
#include "Utils.h"
#include <stdexcept>


using namespace std;
using namespace commons;

shared_ptr<Config> ConfigFileSingleton::instance = nullptr;


ConfigFile::ConfigFile(const string& file_path) : file_path(file_path) {
    this->file_path = file_path;
    this->load();
}


void ConfigFile::load() {
    this->config_map = Utils::parse_config(this->file_path);
}

string ConfigFile::get(const string& key) const {
    if (this->config_map.find(key) != this->config_map.end()) {
        return this->config_map.at(key);
    } else {
        throw runtime_error("Key not found in configuration: " + key);
    }
}


void ConfigFileSingleton::init(const string& file_path) {
    if (ConfigFileSingleton::instance == nullptr) {
        ConfigFileSingleton::instance = make_shared<ConfigFile>(file_path);
    } else {
        Utils::warning("ConfigFileSingleton is already initialized. Cannot reinitialize.");
    }
}

shared_ptr<Config> ConfigFileSingleton::get_instance() {
    return ConfigFileSingleton::instance;
}
