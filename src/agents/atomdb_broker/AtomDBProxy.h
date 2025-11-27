#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "Atom.h"
#include "AtomDB.h"
#include "BaseProxy.h"
#include "SharedQueue.h"
#include "ThreadPool.h"

using namespace std;
using namespace agents;
using namespace atoms;
using namespace atomdb;

namespace atomdb_broker {

/**
 * @brief Proxy between local clients and the AtomDB broker service.
 *
 * This class implements the client and server sides of a proxy that sends
 * commands to an AtomDB broker (remote peer) and receives incoming commands
 * from remote peers.
 */
class AtomDBProxy : public BaseProxy {
   public:
    // ---------------------------------------------------------------------------------------------
    // Proxy Commands
    static int THREAD_POOL_SIZE;
    static size_t MAX_PENDING_ATOMS;
    static string ADD_ATOMS;
    static string START_STREAM;
    static string END_STREAM;
    static string NODE;
    static string LINK;

    // ---------------------------------------------------------------------------------------------
    // Constructor and destructor

    /**
     * @brief Constructor.
     */
    AtomDBProxy();

    /**
     * @brief Destructor.
     */
    virtual ~AtomDBProxy();

    // ---------------------------------------------------------------------------------------------
    // Client-side API

    /**
     * @brief Packs the internal args vector for an outgoing RPC.
     */
    virtual void pack_command_line_args() override;

    /**
     * @brief Write a tokenized representation of this proxy in the passed `output` vector.
     *
     * @param output Vector where the tokens will be put.
     */
    virtual void tokenize(vector<string>& output) override;

    /**
     * @brief Send atoms to the remote peer and return their local handles.
     *
     * This serializes each atom into the RPC arguments and issues an
     * ADD_ATOMS command to the remote peer. The returned vector contains the
     * handle of each atom in the same order as the input.
     *
     * @param atoms Vector of pointers to Atom to be sent.
     * @return Vector of handles corresponding to each atom.
     */
    vector<string> add_atoms(const vector<Atom*>& atoms, bool use_streaming = false);

    vector<string> add_atoms(const vector<string>& tokens, bool use_streaming = false);

    // ---------------------------------------------------------------------------------------------
    // server-side API

    /**
     * @brief Process an incoming command from a remote peer.
     *
     * Returns true if the command was handled successfully.
     */
    virtual bool from_remote_peer(const string& command, const vector<string>& args) override;

    /**
     * @brief Reconstruct proxy state from tokens.
     *
     * Called when an untokenize operation targets this proxy.
     */
    virtual void untokenize(vector<string>& tokens) override;

   private:
    /**
     * @brief Handle an incoming ADD_ATOMS command from a remote peer.
     *
     * This will build Atom objects from the tokens and apply them to the
     * local AtomDB instance. Any exception during processing will be
     * reported back to the peer.
     */

    void handle_add_atoms(const vector<string>& args);
    template <typename AtomDataType, typename Factory>
    std::vector<AtomDataType> build_atoms_from_tokens(const vector<string>& tokens, Factory&& factory) {
        std::vector<AtomDataType> atoms;
        std::string current;
        std::vector<std::string> buffer;

        auto flush = [&]() {
            if (current.empty()) return;
            atoms.emplace_back(factory(current, buffer));
            buffer.clear();
        };

        for (const auto& t : tokens) {
            if (t == NODE || t == LINK) {
                if (!current.empty()) flush();
                current = t;
            } else {
                buffer.push_back(t);
            }
        }

        if (!current.empty()) flush();

        return atoms;
    }
    static std::shared_ptr<Atom> shared_ptr_atom_factory(const string& type, vector<string>& data) {
        if (type == NODE) return std::make_shared<Node>(data);
        return std::make_shared<Link>(data);
    }
    static Atom* raw_ptr_atom_factory(const string& type, vector<string>& data) {
        if (type == NODE) return new Node(data);
        return new Link(data);
    }
    void enqueue_request(const vector<string>& tokens);
    void process_atom_batches();
    void set_stream(const string& command, const vector<string>& args);

    mutex api_mutex;
    shared_ptr<AtomDB> atomdb;

    static const size_t BATCH_SIZE;
    static const size_t COMMAND_SIZE_LIMIT;
    bool is_processing_buffer = false;
    SharedQueue* processing_queue = nullptr;
    size_t pending_atoms_count = 0;
    thread processing_thread;
    ThreadPool* thread_pool;
    bool stop_processing = false;
};

}  // namespace atomdb_broker
