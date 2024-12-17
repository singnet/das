#include <cstring>

#include "Utils.h"
#include "expression_hasher.h"
#include "AtomDBAPITypes.h"

using namespace query_engine;
using namespace atomdb_api_types;
using namespace commons;

RedisSet::RedisSet(redisReply *reply) : HandleList() {
    this->redis_reply = reply;
    this->handles_size = reply->elements;
    this->handles = new char *[this->handles_size];
    for (unsigned int i = 0; i < this->handles_size; i++) {
        handles[i] = reply->element[i]->str;
    }
}

RedisSet::~RedisSet() {
    delete [] this->handles;
    freeReplyObject(this->redis_reply);
}

const char *RedisSet::get_handle(unsigned int index) {
    if (index > this->handles_size) {
        Utils::error("Handle index out of bounds: " + to_string(index) + " Answer array size: " + to_string(this->handles_size));
    }
    return handles[index];
}

unsigned int RedisSet::size() {
    return this->handles_size;
}

RedisStringBundle::RedisStringBundle(redisReply *reply) : HandleList() {
    unsigned int handle_length = (HANDLE_HASH_SIZE - 1);
    this->redis_reply = reply;
    this->handles_size = reply->len / handle_length;
    this->handles = new char *[this->handles_size];
    for (unsigned int i = 0; i < this->handles_size; i++) {
        handles[i] = strndup(reply->str + (i * handle_length), handle_length);
    }
}

RedisStringBundle::~RedisStringBundle() {
    for (unsigned int i = 0; i < this->handles_size; i++) {
        free(this->handles[i]);
    }
    delete [] this->handles;
    freeReplyObject(this->redis_reply);
}

const char *RedisStringBundle::get_handle(unsigned int index) {
    if (index > this->handles_size) {
        Utils::error("Handle index out of bounds: " + to_string(index) + " Answer handles size: " + to_string(this->handles_size));
    }
    return handles[index];
}

unsigned int RedisStringBundle::size() {
    return this->handles_size;
}

MongodbDocument::MongodbDocument(core::v1::optional<bsoncxx::v_noabi::document::value>& document) {
    this->document = document;
}

MongodbDocument::~MongodbDocument() {
}

const char *MongodbDocument::get(const string &key) {
    // Note for reference: .to_string() instead of .data() would return a std::string
    return ((*this->document)[key]).get_string().value.data();
}

const char *MongodbDocument::get(const string &array_key, unsigned int index) {
    // Note for reference: .to_string() instead of .data() would return a std::string
    return ((*this->document)[array_key]).get_array().value[index].get_string().value.data();
}

unsigned int MongodbDocument::get_size(const string &array_key) {
    // NOTE TO REVIEWER 
    // TODO: this implementation is wrong and need to be fixed before integration in das-atom-db
    // I couldn't figure out a way to discover the number of elements in a BSON array.
    //cout << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" << endl;
    //cout << "XXXXXXXXXXXXXXXXXXX MongodbDocument::get_size()" << endl;
    //cout << "XXXXXXXXXXXXXXXXXXX MongodbDocument::get_size() length: " << ((*this->document)[array_key]).get_array().value.length() << endl;
    //cout << "XXXXXXXXXXXXXXXXXXX MongodbDocument::get_size() HASH: " << HANDLE_HASH_SIZE << endl;
    //cout << "XXXXXXXXXXXXXXXXXXX MongodbDocument::get_size() size: " << ((*this->document)[array_key]).get_array().value.length() / HANDLE_HASH_SIZE << endl;
    return ((*this->document)[array_key]).get_array().value.length() / HANDLE_HASH_SIZE;
}
