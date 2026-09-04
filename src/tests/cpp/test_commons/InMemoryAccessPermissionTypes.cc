#include "InMemoryAccessPermissionTypes.h"

#include "Utils.h"

using namespace atomdb;
using namespace std;

InMemoryAccessPermissionEntry::InMemoryAccessPermissionEntry(vector<string> tokens,
                                                             bool read,
                                                             bool write)
    : tokens_(std::move(tokens)), read_(read), write_(write) {}

bool InMemoryAccessPermissionEntry::get_read() const { return this->read_; }

bool InMemoryAccessPermissionEntry::get_write() const { return this->write_; }

unsigned int InMemoryAccessPermissionEntry::get_tokens_size() const {
    return static_cast<unsigned int>(this->tokens_.size());
}

const char* InMemoryAccessPermissionEntry::get_token(unsigned int index) const {
    if (index >= this->tokens_.size()) {
        RAISE_ERROR("Access permission entry token index out of bounds: " + to_string(index));
    }
    return this->tokens_[index].c_str();
}

InMemoryAccessPermissionDocument::InMemoryAccessPermissionDocument() : full_access_(false) {}

void InMemoryAccessPermissionDocument::set_access_key(const string& access_key) {
    this->access_key_ = access_key;
}

void InMemoryAccessPermissionDocument::set_full_access(bool full_access) {
    this->full_access_ = full_access;
}

void InMemoryAccessPermissionDocument::append_entry(const vector<string>& tokens,
                                                    bool read,
                                                    bool write) {
    this->entries_.emplace_back(tokens, read, write);
}

const char* InMemoryAccessPermissionDocument::get_access_key() const {
    return this->access_key_.c_str();
}

bool InMemoryAccessPermissionDocument::get_full_access() const { return this->full_access_; }

unsigned int InMemoryAccessPermissionDocument::get_entries_size() const {
    return static_cast<unsigned int>(this->entries_.size());
}

const atomdb_api_types::AccessPermissionEntry& InMemoryAccessPermissionDocument::get_entry(
    unsigned int index) const {
    if (index >= this->entries_.size()) {
        RAISE_ERROR("Access permission entry index out of bounds: " + to_string(index));
    }
    return this->entries_[index];
}
