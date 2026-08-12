#pragma once

#include <string>
#include <vector>

#include "LinkSchema.h"

using namespace std;
using namespace atoms;

namespace atomdb {

enum class AuthorizationOperation { READ, WRITE };

/**
 * @brief One LinkSchema entry with independent read/write flags.
 *
 * Maps to one MongoDB allowed_schemas item. handle() is the LinkSchema handle, computed the same
 * way an Atom handle is, and it is what identifies this entry in AuthorizationManagement::revoke().
 */
class AuthorizationEntry {
   public:
    /** @brief Builds an entry from a LinkSchema. */
    AuthorizationEntry(const LinkSchema& schema, bool read, bool write);

    /** @brief Builds an entry from the tokens stored in MongoDB. */
    AuthorizationEntry(const vector<string>& tokens, bool read, bool write);

    /** @brief Handle of the underlying LinkSchema. */
    string handle() const;

    /** @brief Returns the underlying LinkSchema. */
    const LinkSchema& schema() const;

    /** @brief Returns true if this entry allows the given operation. */
    bool allows(AuthorizationOperation operation) const;

    /** @brief Returns a string representation of the entry. */
    string to_string() const;

    /** @brief Returns the tokens of the underlying LinkSchema. */
    vector<string> tokenize() const;

   private:
    LinkSchema _schema;
    bool _read;
    bool _write;
};

}  // namespace atomdb
