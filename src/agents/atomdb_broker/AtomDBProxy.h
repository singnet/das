#pragma once

#include <mutex>
#include <memory>
#include <string>
#include <vector>

#include "Atom.h"
#include "AtomDB.h"
#include "BaseProxy.h"

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
    static string ADD_ATOMS;

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
    * @brief Build Atom objects from a token stream.
    *
    * The returned vector owns newly-allocated Atom pointers; the caller is
    * responsible for their lifetime (matching the previous behaviour).
    * 
    * * Token format expected by this method:
    *
    * <NODE> <type> <is_toplevel> <number_properties> <property_name> <property_type> <property_value> <name>
    * <LINK> <type> <is_toplevel> <number_properties> <property_name> <property_type> <property_value> <number_handles> <handle_1> ... <handle_n>
    * 
    * If there are no properties <number_properties> should be 0
    *
    * @param tokens Token stream describing one or more atoms (NODE/LINK blocks)
    * @return vector of Atom* reconstructed from tokens.
    */
    vector<Atom*> build_atoms_from_tokens(const vector<string>& tokens);

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
    vector<string> add_atoms(const vector<Atom*>& atoms);

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

    mutex api_mutex;
    shared_ptr<AtomDB> atomdb;
};

}  // namespace atomdb_broker
