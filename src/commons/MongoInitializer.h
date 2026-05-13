#pragma once
#include <mongocxx/instance.hpp>

class MongoInitializer {
   public:
    static void initialize() { static mongocxx::instance instance{}; }
};