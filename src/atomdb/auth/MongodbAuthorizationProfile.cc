#include "MongodbAuthorizationProfile.h"

#include "Hasher.h"
#include "Utils.h"

using namespace atomdb;
using namespace std;
using namespace commons;

MongodbAuthorizationProfile::MongodbAuthorizationProfile() : full_access_(false) {}

void MongodbAuthorizationProfile::set_access_key(const string& access_key) {
    this->access_key_ = access_key;
}

void MongodbAuthorizationProfile::set_full_access(bool full_access) { this->full_access_ = full_access; }

void MongodbAuthorizationProfile::append_entry(const vector<string>& tokens, bool read, bool write) {
    auto tokens_array = bsoncxx::builder::basic::array{};
    for (const auto& token : tokens) {
        tokens_array.append(token);
    }
    this->schemas_.append(
        bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("tokens", tokens_array),
                                               bsoncxx::builder::basic::kvp("read", read),
                                               bsoncxx::builder::basic::kvp("write", write)));
}

const string& MongodbAuthorizationProfile::get_access_key() const { return this->access_key_; }

bsoncxx::document::value MongodbAuthorizationProfile::value() const {
    if (this->access_key_.empty()) {
        RAISE_ERROR("MongodbAuthorizationProfile missing access key");
    }

    return bsoncxx::builder::basic::make_document(
        bsoncxx::builder::basic::kvp("_id", Hasher::plain_string_hash(this->access_key_)),
        bsoncxx::builder::basic::kvp("public_key", this->access_key_),
        bsoncxx::builder::basic::kvp("full_access", this->full_access_),
        bsoncxx::builder::basic::kvp("allowed_schemas", this->schemas_));
}
