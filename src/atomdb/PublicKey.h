#pragma once

#include <map>
#include <string>

using namespace std;

namespace atomdb {

/**
 * @brief Encapsulates public-key material for protected AtomDB access.
 *
 * Holds either a single public key or a peer-id -> key map.
 * How RemoteAtomDB consumes the map is still TBD; this type only carries the data.
 */
class PublicKey {
   public:
    explicit PublicKey(const string& key);
    explicit PublicKey(const map<string, string>& keys);

    bool is_single_key() const;
    bool is_peer_map() const;

    /** @brief Single key. Raises if this instance holds a peer map. */
    const string& key() const;

    /** @brief Peer-id -> key map. Raises if this instance holds a single key. */
    const map<string, string>& keys() const;

   private:
    string single_key;
    map<string, string> peer_keys;
};

}  // namespace atomdb
