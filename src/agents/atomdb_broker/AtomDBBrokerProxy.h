#pragma once

#include <mutex>
#include <memory>
#include <string>
#include <vector>

#include "AtomDB.h"
#include "Atom.h"
#include "BaseProxy.h"

using namespace std;
using namespace agents;
using namespace atoms;
using namespace atomdb;


namespace atomdb_broker {

class AtomDBBrokerProxy : public BaseProxy {
   public:
    // ---------------------------------------------------------------------------------------------
    // Constructors, destructors and static state

    // Proxy Commands
    static string ADD_ATOMS;
    static string ADD_ATOMS_RESPONSE;
    static string ADD_ATOMS_ERROR;
    static string SHUTDOWN;

    /**
     * Empty constructor.
     */
    AtomDBBrokerProxy();

    /**
     * Destructor.
     */
    virtual ~AtomDBBrokerProxy();

    /**
     * Check if the proxy is still running.
     * @return Whether the proxy is still running
     */
    bool running();

    /**
     * Send a shutdown command to the remote proxy.
     */
    void shutdown();

    // ---------------------------------------------------------------------------------------------
    // Client-side API
    virtual void pack_command_line_args() override;
    virtual void tokenize(vector<string>& output) override;

    vector<Atom*> build_atoms_from_tokens(const vector<string>& tokens);
    vector<string> add_atoms(const vector<Atom*>& atoms);

    // ---------------------------------------------------------------------------------------------
    // server-side API

    virtual bool from_remote_peer(const string& command, const vector<string>& args) override;
    virtual void untokenize(vector<string>& tokens) override;

   private:
    void piggyback_add_atoms(const vector<string>& args);
    
    vector<string> add_atoms_response;
    mutex api_mutex;
    shared_ptr<AtomDB> atomdb;

    bool keep_alive_flag;
    bool add_atoms_response_flag;
    bool add_atoms_error_flag;
};

}  // namespace atomdb_broker
