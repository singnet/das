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
    static string SHUTDOWN;

    /**
     * Empty constructor.
     */
    AtomDBBrokerProxy();
    AtomDBBrokerProxy(string name);
    /**
     * Destructor.
     */
    virtual ~AtomDBBrokerProxy();

    const string get_action();

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

    vector<Atom*> build_atoms_from_tokens(vector<string> tokens);
    virtual void pack_command_line_args() override;
    virtual void tokenize(vector<string>& output) override;
    vector<string> add_atoms(const vector<Atom*>& atoms);

    // ---------------------------------------------------------------------------------------------
    // server-side API

    virtual bool from_remote_peer(const string& command, const vector<string>& args) override;
    virtual void untokenize(vector<string>& tokens) override;

   private:
    vector<string> piggyback_add_atoms(vector<string> args);
    
    vector<string> tokens;
    bool keep_alive;
    mutex api_mutex;
    shared_ptr<AtomDB> atomdb;
};

}  // namespace atomdb_broker
