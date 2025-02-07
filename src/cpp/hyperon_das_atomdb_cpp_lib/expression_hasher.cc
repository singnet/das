#include "expression_hasher.h"

#include "mbedtls/md5.h"

using namespace std;

namespace atomdb {

//------------------------------------------------------------------------------
const string ExpressionHasher::compute_hash(const string& input) {
    auto ctx = unique_ptr<mbedtls_md5_context, decltype(&mbedtls_md5_free)>(new mbedtls_md5_context(),
                                                                            &mbedtls_md5_free);
    mbedtls_md5_init(ctx.get());
    uchar md5_buffer[MD5_BUFFER_SIZE];
    try {
        mbedtls_md5_starts(ctx.get());
        mbedtls_md5_update(ctx.get(), (const uchar*) input.c_str(), input.length());
        mbedtls_md5_finish(ctx.get(), md5_buffer);
    } catch (...) {
        throw runtime_error("Failed to compute MD5 hash");
    }
    char hash[2 * MD5_BUFFER_SIZE + 1];
    for (unsigned int i = 0; i < MD5_BUFFER_SIZE; i++) {
        sprintf(hash + 2 * i, "%02x", md5_buffer[i]);
    }
    hash[2 * MD5_BUFFER_SIZE] = '\0';
    return move(string(hash));
}

//------------------------------------------------------------------------------
const string ExpressionHasher::named_type_hash(const string& name) { return compute_hash(name); }

//------------------------------------------------------------------------------
const string ExpressionHasher::terminal_hash(const string& type, const string& name) {
    if (type.length() + name.length() >= MAX_HASHABLE_STRING_SIZE) {
        throw invalid_argument("Invalid (too large) terminal name");
    }
    string hashable_string = type + JOINING_CHAR + name;
    return move(compute_hash(hashable_string));
}

//------------------------------------------------------------------------------
const string ExpressionHasher::composite_hash(const StringList& elements) {
    if (elements.size() == 1) {
        return elements[0];
    }

    string hashable_string;
    if (not elements.empty()) {
        for (const auto& element : elements) {
            hashable_string += element + JOINING_CHAR;
        }
        hashable_string.pop_back();  // remove the last joining character
    }

    return move(compute_hash(hashable_string));
}

//------------------------------------------------------------------------------
const string ExpressionHasher::composite_hash(const string& hash_base) { return hash_base; }

//------------------------------------------------------------------------------
const string ExpressionHasher::expression_hash(const string& type_hash, const StringList& elements) {
    StringList composite({type_hash});
    composite.insert(composite.end(), elements.begin(), elements.end());
    return move(composite_hash(composite));
}

}  // namespace atomdb
