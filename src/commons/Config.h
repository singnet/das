#pragma once

#include <map>
#include <string>

using namespace std;

namespace commons {

    /***
     * Enum like class to represent configuration keys.
     * This class is intended to be used as a namespace for configuration keys.
     * It provides a common interface for accessing configuration keys.
     */
    class ConfigKeys {
    public:
        class Agents{
            public:
            class LinkCreation {
            public:
                static const string REQUESTS_INTERVAL;  // Default interval to send requests
                static const string THREAD_COUNT;  // Number of threads to process requests
                static const string QUERY_SERVER;  // Server ID for the Query Agent
                static const string QUERY_TIMEOUT; // Timeout for query requests
                static const string SERVER_ID;  // Server ID for the Link Creation
                static const string BUFFER_FILE;  // File to store requests buffer
                static const string METTA_FILE_PATH;  // Path to the Metta file
                static const string SAVE_TO_METTA_FILE;  // Flag to save links to
                static const string SAVE_TO_DB;  // Flag to save links to the database
                static const string QUERY_START_PORT;  // Start port for the Query Agent client
                static const string QUERY_END_PORT;  // End port for the Query Agent client
                };

        };
        // Add more configuration keys as needed
    };

    /**
     * Configuration abstract class.
     * This class is intended to be used as a base class for configuration classes.
     * It provides a common interface for loading configurations.
     */
    class Config {
    public:
        /**
         * Virtual destructor.
         * Ensures that derived classes can be properly cleaned up.
         */
        virtual ~Config() = default;
        /**
         * Load configuration from a file.
         * This method should be implemented by derived classes to load their specific configurations.
         *
         * @param file_path Path to the configuration file.
         */
        virtual void load() = 0;


        /**
         * Get a configuration value by key.
         * This method should be implemented by derived classes to retrieve specific configuration values.
         *
         * @param key The key of the configuration value to retrieve.
         * @return The configuration value associated with the given key.
         */
        virtual string get(const string& key) const = 0;

    };
} // namespace commons