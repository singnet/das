#pragma once

#include <memory>
#include <string>
#include <vector>

#include "AtomDBAPITypes.h"

using namespace std;

namespace atomdb {

class InMemoryAccessPermissionEntry : public atomdb_api_types::AccessPermissionEntry {
   public:
    InMemoryAccessPermissionEntry(vector<string> tokens, bool read, bool write);
    ~InMemoryAccessPermissionEntry() override = default;

    bool get_read() const override;
    bool get_write() const override;
    unsigned int get_tokens_size() const override;
    const char* get_token(unsigned int index) const override;

   private:
    vector<string> tokens_;
    bool read_;
    bool write_;
};

class InMemoryAccessPermissionDocument : public atomdb_api_types::AccessPermissionDocument {
   public:
    InMemoryAccessPermissionDocument();
    ~InMemoryAccessPermissionDocument() override = default;

    void set_access_key(const string& access_key);
    void set_full_access(bool full_access);
    void append_entry(const vector<string>& tokens, bool read, bool write);

    const char* get_access_key() const override;
    bool get_full_access() const override;
    unsigned int get_entries_size() const override;
    const atomdb_api_types::AccessPermissionEntry& get_entry(unsigned int index) const override;

   private:
    string access_key_;
    bool full_access_;
    vector<InMemoryAccessPermissionEntry> entries_;
};

}  // namespace atomdb
