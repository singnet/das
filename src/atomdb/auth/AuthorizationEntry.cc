#include "AuthorizationEntry.h"

using namespace atomdb;

// --------------------------------------------------------------------------------
// Constructors

AuthorizationEntry::AuthorizationEntry(const LinkSchema& schema, bool read, bool write)
    : _schema(schema), _read(read), _write(write) {}

AuthorizationEntry::AuthorizationEntry(const vector<string>& tokens, bool read, bool write)
    : _schema(tokens), _read(read), _write(write) {}

// --------------------------------------------------------------------------------
// Public methods

string AuthorizationEntry::handle() const { return this->_schema.handle(); }

const LinkSchema& AuthorizationEntry::schema() const { return this->_schema; }

bool AuthorizationEntry::allows(AuthorizationOperation operation) const {
    switch (operation) {
        case AuthorizationOperation::READ:
            return this->_read;
        case AuthorizationOperation::WRITE:
            return this->_write;
    }
    return false;
}

string AuthorizationEntry::to_string() const {
    return "AuthorizationEntry(handle: '" + this->handle() +
           "', read: " + (this->_read ? "true" : "false") +
           ", write: " + (this->_write ? "true" : "false") + ", schema: " + this->_schema.to_string() +
           ")";
}

vector<string> AuthorizationEntry::tokenize() const {
    LinkSchema schema_copy = this->_schema;
    return schema_copy.tokenize();
}
