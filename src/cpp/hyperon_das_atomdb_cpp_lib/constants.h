#pragma once

#include "expression_hasher.h"
#include "type_aliases.h"

using namespace std;

namespace atomdb {

constexpr const char* WILDCARD = "*";

// pre-computed hashes
static const string WILDCARD_HASH = ExpressionHasher::named_type_hash(WILDCARD);
static const string TYPE_HASH = ExpressionHasher::named_type_hash("Type");
static const string TYPEDEF_MARK_HASH = ExpressionHasher::named_type_hash(":");

enum FieldIndexType { BINARY_TREE = 0, TOKEN_INVERTED_LIST };

struct FieldNames {
    static const char* ID_HASH;
    static const char* HANDLE;
    static const char* COMPOSITE_TYPE;
    static const char* COMPOSITE_TYPE_HASH;
    static const char* NODE_NAME;
    static const char* TYPE_NAME;
    static const char* TYPE_NAME_HASH;
    static const char* KEY_PREFIX;
    static const char* KEYS;
    static const char* IS_TOPLEVEL;
    static const char* TARGETS;
    static const char* TARGETS_DOCUMENTS;
    static const char* CUSTOM_ATTRIBUTES;
};

}  // namespace atomdb