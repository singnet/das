#include "RedisMongoDBAPITypes.h"

#include <cstring>

#include "Utils.h"
#include "expression_hasher.h"

using namespace atomdb;
using namespace atomdb_api_types;
using namespace commons;
using namespace std;

HandleSetRedis::HandleSetRedis(bool delete_replies_on_destruction) : HandleSet() {
    this->handles_size = 0;
    this->delete_replies_on_destruction = delete_replies_on_destruction;
}

HandleSetRedis::HandleSetRedis(redisReply* reply, bool delete_replies_on_destruction) : HandleSet() {
    this->replies.push_back(reply);
    this->handles_size = reply->elements;
    this->delete_replies_on_destruction = delete_replies_on_destruction;
}

HandleSetRedis::~HandleSetRedis() {
    if (this->delete_replies_on_destruction) {
        for (auto reply : this->replies) {
            freeReplyObject(reply);
        }
    }
}

void HandleSetRedis::append(shared_ptr<HandleSet> other) {
    auto handle_set_redis = dynamic_pointer_cast<HandleSetRedis>(other);
    for (auto reply : handle_set_redis->replies) {
        this->replies.push_back(reply);
        this->handles_size += reply->elements;
    }
}

unsigned int HandleSetRedis::size() { return this->handles_size; }

shared_ptr<HandleSetIterator> HandleSetRedis::get_iterator() {
    shared_ptr<HandleSetRedisIterator> it(new HandleSetRedisIterator(this));
    return it;
}

HandleSetRedisIterator::HandleSetRedisIterator(HandleSetRedis* handle_set) {
    this->handle_set = handle_set;
    this->outer_idx = 0;
    this->inner_idx = 0;
};

HandleSetRedisIterator::~HandleSetRedisIterator(){};

char* HandleSetRedisIterator::next() {
    if (this->outer_idx >= this->handle_set->replies.size()) {
        // No more elements, reset indexes and return nullptr.
        return nullptr;
    }

    redisReply* reply = this->handle_set->replies[this->outer_idx];

    // If inner_idx exists, get its element and bump it.
    if (this->inner_idx < reply->elements) {
        return reply->element[this->inner_idx++]->str;
    }

    // If not, point to the next outer_idx (redisReply)
    this->outer_idx++;
    this->inner_idx = 0;

    // Get first element of the next redisReply
    if (this->outer_idx < this->handle_set->replies.size()) {
        reply = this->handle_set->replies[this->outer_idx];
        if (this->inner_idx < reply->elements) {
            return reply->element[this->inner_idx++]->str;
        }
    }

    // No more elements, reset indexes and return nullptr
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

MongodbDocument::MongodbDocument(
    bsoncxx::v_noabi::stdx::optional<bsoncxx::v_noabi::document::value>& document) {
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

bool MongodbDocument::contains(const string& key) {
    return ((*this->document).view()).find(key) != ((*this->document).view()).end();
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

bsoncxx::v_noabi::document::value MongodbDocument::value() { return this->document.value(); }

void append_custom_attributes(bsoncxx::builder::basic::document& doc,
                              const Properties& custom_attributes) {
    for (const auto& [key, value] : custom_attributes) {
        visit(
            [&](auto&& arg) {
                using T = decay_t<decltype(arg)>;
                if constexpr (is_same_v<T, string>) {
                    doc.append(bsoncxx::builder::basic::kvp(key, arg));
                } else if constexpr (is_same_v<T, long>) {
                    doc.append(bsoncxx::builder::basic::kvp(key, static_cast<int64_t>(arg)));
                } else if constexpr (is_same_v<T, double>) {
                    doc.append(bsoncxx::builder::basic::kvp(key, arg));
                } else if constexpr (is_same_v<T, bool>) {
                    doc.append(bsoncxx::builder::basic::kvp(key, arg));
                }
            },
            value);
    }
}

MongodbDocument::MongodbDocument(const atomspace::Node* node) {
    char* handle = atomspace::Node::compute_handle(node->type, node->name);

    const char* elements[1] = {node->type.c_str()};

    bsoncxx::builder::basic::document doc;
    doc.append(bsoncxx::builder::basic::kvp("_id", handle));
    doc.append(bsoncxx::builder::basic::kvp("composite_type_hash",
                                            composite_hash(const_cast<char**>(elements), 1)));
    doc.append(bsoncxx::builder::basic::kvp("name", node->name));
    doc.append(bsoncxx::builder::basic::kvp("named_type", node->type));
    append_custom_attributes(doc, node->custom_attributes);
    this->document = doc.extract();
}

MongodbDocument::MongodbDocument(const atomspace::Link* link) {
    char* handle = atomspace::Link::compute_handle(link->type, link->targets);

    const char* named_type_hash_elements[1] = {link->type.c_str()};
    char* named_type_hash = composite_hash(const_cast<char**>(named_type_hash_elements), 1);

    const char* elements[link->targets.size() + 1];
    elements[0] = named_type_hash;

    bsoncxx::builder::basic::array composite_type;
    composite_type.append(named_type_hash);
    for (size_t i = 0; i < link->targets.size(); i++) {
        const char* type_hash_elements[1] = {link->targets[i]->type.c_str()};
        auto type_hash = composite_hash(const_cast<char**>(type_hash_elements), 1);
        elements[i + 1] = type_hash;
        composite_type.append(type_hash);
    }

    bsoncxx::builder::basic::array targets;
    auto targets_hashes = atomspace::Link::targets_to_handles(link->targets);
    for (size_t i = 0; i < link->targets.size(); i++) {
        targets.append(targets_hashes[i]);
    }

    bsoncxx::builder::basic::document doc;
    doc.append(bsoncxx::builder::basic::kvp("_id", handle));
    doc.append(bsoncxx::builder::basic::kvp(
        "composite_type_hash", composite_hash(const_cast<char**>(elements), link->targets.size() + 1)));
    doc.append(bsoncxx::builder::basic::kvp("composite_type", composite_type));
    doc.append(bsoncxx::builder::basic::kvp("named_type", link->type));
    doc.append(bsoncxx::builder::basic::kvp("named_type_hash", named_type_hash));
    doc.append(bsoncxx::builder::basic::kvp("targets", targets));
    append_custom_attributes(doc, link->custom_attributes);
    this->document = doc.extract();
}
