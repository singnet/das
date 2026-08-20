#include "ManifestAuthorizer.h"

#include "Assignment.h"
#include "Atom.h"
#include "AtomDB.h"
#include "Link.h"
#include "Utils.h"

using namespace atomdb;
using namespace atoms;
using namespace commons;

// --------------------------------------------------------------------------------
// Constructor

ManifestAuthorizer::ManifestAuthorizer(shared_ptr<AuthorizationManifest> manifest)
    : manifest(manifest) {}

// --------------------------------------------------------------------------------
// Public methods

bool ManifestAuthorizer::is_authorized(const Atom& atom,
                                       const string& public_key,
                                       AuthorizationOperation operation,
                                       HandleDecoder& decoder) {
    if (!this->manifest->is_registered(public_key)) {
        return false;
    }
    if (this->manifest->full_access(public_key)) {
        return true;
    }

    auto document = this->manifest->get_document(public_key);
    for (const auto& entry : document->entries) {
        if (this->allows(entry, operation) && this->matches_schema(entry.schema, atom, decoder)) {
            return true;
        }
    }
    return false;
}

bool ManifestAuthorizer::is_authorized(const string& handle,
                                       const string& public_key,
                                       AuthorizationOperation operation,
                                       HandleDecoder& decoder) {
    if (!this->manifest->is_registered(public_key)) {
        return false;
    }
    if (this->manifest->full_access(public_key)) {
        return true;
    }

    auto document = this->manifest->get_document(public_key);
    for (const auto& entry : document->entries) {
        if (allows(entry, operation) && this->matches_schema(entry.schema, handle, decoder)) {
            return true;
        }
    }
    return false;
}

// --------------------------------------------------------------------------------
// Private methods

bool ManifestAuthorizer::allows(const atomdb_api_types::AccessPermissionEntry& entry,
                                AuthorizationOperation operation) {
    switch (operation) {
        case AuthorizationOperation::READ:
            return entry.read;
        case AuthorizationOperation::WRITE:
            return entry.write;
    }
    return false;
}

bool ManifestAuthorizer::matches_schema(const LinkSchema& schema,
                                        const Atom& atom,
                                        HandleDecoder& decoder) const {
    Assignment assignment;
    LinkSchema local_schema(schema);
    if (Atom::is_link(atom)) {
        auto& link = const_cast<Link&>(static_cast<const Link&>(atom));
        return local_schema.match(link, assignment, decoder);
    }
    return local_schema.match(atom.handle(), assignment, decoder);
}

bool ManifestAuthorizer::matches_schema(const LinkSchema& schema,
                                        const string& handle,
                                        HandleDecoder& decoder) const {
    Assignment assignment;
    LinkSchema local_schema(schema);
    return local_schema.match(handle, assignment, decoder);
}