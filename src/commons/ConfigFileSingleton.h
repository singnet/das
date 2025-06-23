/***
 * @file ConfigFileSingleton.h
 * @brief Singleton class to manage configuration file loading and access
 */

#pragma once
#include "Config.h"
#include <memory>

namespace commons {

    class ConfigFile : public Config {
    public:
        /**
         * Constructor that initializes the configuration file path.
         * @param file_path Path to the configuration file.
         */
        ConfigFile(const string& file_path);
        /**
         * Load configuration from the specified file.
         * This method reads the file and populates the config_map with key-value pairs.
         */
        void load() override;
        /**
         * Get a configuration value by key.
         * This method retrieves the value associated with the given key from the config_map.
         * @param key The key of the configuration value to retrieve.
         * @return The configuration value associated   with the given key.
         */
        string get(const string& key) const override;
        /**
         * Get the file path of the configuration file.
         * @return The file path of the configuration file.
         */
        
    private:
        string file_path;  // Path to the configuration file
        map<string, string> config_map;  // Map to store configuration key-value pairs
    };

    class ConfigFileSingleton  {
    public:
        /**
         * Static method to initialize the singleton instance with a file path.
         *
         * @param file_path Path to the configuration file.
         */
        static void init(const string& file_path);
        /**
         * Get the singleton instance of ConfigFileSingleton.
         *
         * @return The singleton instance of ConfigFileSingleton.
         */
        static shared_ptr<Config> get_instance();


    private:
        ConfigFileSingleton(){}
        string file_path;  // Path to the configuration file
        map<string, string> config_map;  // Map to store configuration key-value pairs
        static shared_ptr<Config> instance;  // Static instance of ConfigFileSingleton
    };
} // namespace commons