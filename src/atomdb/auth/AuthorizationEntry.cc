#include "AuthorizationEntry.h"

using namespace atomdb;

// --------------------------------------------------------------------------------
// Constructors

AuthorizationEntry::AuthorizationEntry(const LinkSchema& schema, bool read, bool write)
    : schema(schema), read(read), write(write) {}

AuthorizationEntry::AuthorizationEntry(const vector<string>& tokens, bool read, bool write)
    : schema(tokens), read(read), write(write) {}

// --------------------------------------------------------------------------------
// Public methods

string AuthorizationEntry::handle() const { return this->schema.handle(); }

const LinkSchema& atomdb::AuthorizationEntry::schema() const { return this->schema; }

bool AuthorizationEntry::allows(AuthorizationOperation operation) const {
    switch (operation) {
        case AuthorizationOperation::READ:
            return this->read;
        case AuthorizationOperation::WRITE:
            return this->write;
    }
    return false;
}

string AuthorizationEntry::to_string() const {
    return "AuthorizationEntry(handle: '" + this->handle() +
           "', read: " + (this->read ? "true" : "false") +
           ", write: " + (this->write ? "true" : "false") + ", schema: " + this->schema.to_string() +
           ")";
}

vector<string> AuthorizationEntry::tokenize() const {
    LinkSchema schema_copy = this->schema;
    return schema_copy.tokenize();
}
