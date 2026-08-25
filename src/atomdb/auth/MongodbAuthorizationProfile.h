#pragma once

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <string>
#include <vector>

using namespace std;

namespace atomdb {

/**
 * @brief Builds MongoDB access-permission documents for authorization persistence.
 */
class MongodbAuthorizationProfile {
   public:
    MongodbAuthorizationProfile();
    ~MongodbAuthorizationProfile() = default;

    void set_access_key(const string& access_key);
    void set_full_access(bool full_access);
    void append_entry(const vector<string>& tokens, bool read, bool write);

    const string& get_access_key() const;
    bsoncxx::document::value value() const;

   private:
    string access_key_;
    bool full_access_;
    bsoncxx::builder::basic::array schemas_;
};

}  // namespace atomdb
