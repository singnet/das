#include "RedisMongoDBAPITypes.h"

#include <cstring>

#include "Utils.h"
#include "expression_hasher.h"

using namespace atomdb;
using namespace atomdb_api_types;
using namespace commons;

HandleSetRedis::HandleSetRedis() : HandleSet() {
    this->handles_size = 0;
    this->outer_idx = 0;
    this->inner_idx = 0;
}

HandleSetRedis::HandleSetRedis(redisReply* reply) : HandleSet() {
    this->replies.push_back(reply);
    this->handles_size = reply->elements;
    this->outer_idx = 0;
    this->inner_idx = 0;
}

HandleSetRedis::~HandleSetRedis() {
    for (auto reply : this->replies) {
        freeReplyObject(reply);
    }
}

void HandleSetRedis::append(void* data) {
    redisReply* reply = static_cast<redisReply*>(data);
    this->handles_size += reply->elements;
    this->replies.push_back(reply);
}

unsigned int HandleSetRedis::size() { return this->handles_size; }

char* HandleSetRedis::next() {
    if (this->outer_idx >= this->replies.size()) {
        // No more elements, reset indexes and return nullptr.
        this->outer_idx = 0;
        this->inner_idx = 0;
        return nullptr;
    }

    redisReply* reply = this->replies[this->outer_idx];

    // If inner_idx exists, get its element and bump it.
    if (this->inner_idx < reply->elements) {
        return reply->element[this->inner_idx++]->str;
    }

    // If not, point to the next outer_idx (redisReply)
    this->outer_idx++;
    this->inner_idx = 0;

    // Get first element of the next redisReply
    if (this->outer_idx < this->replies.size()) {
        reply = this->replies[this->outer_idx];
        return reply->element[this->inner_idx++]->str;
    }

    // No more elements, reset indexes and return nullptr
    this->outer_idx = 0;
    this->inner_idx = 0;
    return nullptr;
}

RedisStringBundle::RedisStringBundle(redisReply* reply) : HandleList() {
    unsigned int handle_length = (HANDLE_HASH_SIZE - 1);
    this->redis_reply = reply;
    this->handles_size = reply->len / handle_length;
    this->handles = new char*[this->handles_size];
    for (unsigned int i = 0; i < this->handles_size; i++) {
        handles[i] = strndup(reply->str + (i * handle_length), handle_length);
    }
}

RedisStringBundle::~RedisStringBundle() {
    for (unsigned int i = 0; i < this->handles_size; i++) {
        free(this->handles[i]);
    }
    delete[] this->handles;
    freeReplyObject(this->redis_reply);
}

const char* RedisStringBundle::get_handle(unsigned int index) {
    if (index > this->handles_size) {
        Utils::error("Handle index out of bounds: " + to_string(index) +
                     " Answer handles size: " + to_string(this->handles_size));
    }
    //
    return handles[index];
}

unsigned int RedisStringBundle::size() { return this->handles_size; }

MongodbDocument::MongodbDocument(core::v1::optional<bsoncxx::v_noabi::document::value>& document) {
    this->document = document;
}

MongodbDocument::~MongodbDocument() {}

const char* MongodbDocument::get(const string& key) {
    // Note for reference: .to_string() instead of .data() would return a std::string
    return ((*this->document)[key]).get_string().value.data();
}

const char* MongodbDocument::get(const string& array_key, unsigned int index) {
    // Note for reference: .to_string() instead of .data() would return a std::string
    return ((*this->document)[array_key]).get_array().value[index].get_string().value.data();
}

unsigned int MongodbDocument::get_size(const string& array_key) {
    // NOTE TO REVIEWER
    // TODO: this implementation is wrong and need to be fixed before integration in das-atom-db
    // I couldn't figure out a way to discover the number of elements in a BSON array.
    // cout << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" << endl;
    // cout << "XXXXXXXXXXXXXXXXXXX MongodbDocument::get_size()" << endl;
    // cout << "XXXXXXXXXXXXXXXXXXX MongodbDocument::get_size() length: " <<
    // ((*this->document)[array_key]).get_array().value.length() << endl; cout << "XXXXXXXXXXXXXXXXXXX
    // MongodbDocument::get_size() HASH: " << HANDLE_HASH_SIZE << endl; cout << "XXXXXXXXXXXXXXXXXXX
    // MongodbDocument::get_size() size: " << ((*this->document)[array_key]).get_array().value.length() /
    // HANDLE_HASH_SIZE << endl;
    return ((*this->document)[array_key]).get_array().value.length() / HANDLE_HASH_SIZE;
}
