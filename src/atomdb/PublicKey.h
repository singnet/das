#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

using namespace std;

namespace atomdb {

class PublicKey {
   public:
    /**
     * @brief Builds a PublicKey from its JSON wire form.
     *
     * Accepts a map, `{"uid_peerA":"key_peerA","uid_peerB":"key_peerB"}`, or a bare JSON string,
     */
    static optional<PublicKey> from_json(const string& json_string);

    explicit PublicKey(const string& key);
    explicit PublicKey(const map<string, string>& peer_keys);
    ~PublicKey() = default;

    inline bool is_single_key() const { return this->peer_to_key.empty(); }
    inline const string& key() const { return this->keys[0]; }

    /**
     * @brief Key assigned to a peer.
     *
     * A single-key PublicKey broadcasts the same key to every peer. A mapped PublicKey
     * looks up `peer_uid` in the map. Returns nullopt when the peer is not in the map.
     */
    optional<string> key_for_peer(const string& peer_uid) const;

    /**
     * @brief Single-key PublicKey to send to a given peer.
     *
     * Returns this object when it is already a single key, or a new single-key PublicKey sliced
     * from the peer-to-key map. Returns nullopt when the peer is not in the map.
     */
    optional<PublicKey> for_peer(const string& peer_uid) const;

    /**
     * @brief Peer uid -> key string entries. Empty when this is a single-key PublicKey.
     */
    map<string, string> peer_keys() const;

   private:
    vector<string> keys;
    map<string, unsigned int> peer_to_key;
};

}  // namespace atomdb
