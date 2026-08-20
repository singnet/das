#pragma once

#include <memory>
#include <string>

#include "Atom.h"
#include "AtomDBAPITypes.h"
#include "AuthorizationManifest.h"
#include "HandleDecoder.h"

using namespace std;
using namespace atoms;

namespace atomdb {

enum class AuthorizationOperation { READ, WRITE };

/**
 * @brief Evaluates authorization requests using an in-memory AuthorizationManifest.
 */
class ManifestAuthorizer {
   public:
    explicit ManifestAuthorizer(shared_ptr<AuthorizationManifest> manifest);

    /**
     * @brief Checks whether public_key is authorized to perform an operation on an atom.
     */
    bool is_authorized(const Atom& atom,
                       const string& public_key,
                       AuthorizationOperation operation,
                       HandleDecoder& decoder);

    /**
     * @brief Checks whether public_key is authorized to perform an operation on a handle.
     */
    bool is_authorized(const string& handle,
                       const string& public_key,
                       AuthorizationOperation operation,
                       HandleDecoder& decoder);

   private:
    shared_ptr<AuthorizationManifest> manifest;

    bool matches_schema(const LinkSchema& schema, const Atom& atom, HandleDecoder& decoder) const;
    bool matches_schema(const LinkSchema& schema, const string& handle, HandleDecoder& decoder) const;

    static bool allows(const atomdb_api_types::AccessPermissionEntry& entry,
                       AuthorizationOperation operation);
};

}  // namespace atomdb
