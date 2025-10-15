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

    /**
     * Empty constructor typically used on server side.
     */
    AtomDBBrokerProxy();

    /**
     * Constructor typically used on client side.
     */
    AtomDBBrokerProxy(const string& action, const vector<string>& tokens);

    /**
     * Destructor.
     */
    virtual ~AtomDBBrokerProxy();

    const string get_action();

    // ---------------------------------------------------------------------------------------------
    // Context-specific API

    virtual bool from_remote_peer(const string& command, const vector<string>& args) override;
    virtual void tokenize(vector<string>& output) override;
    virtual void untokenize(vector<string>& tokens) override;
    // virtual string to_string() override;
    virtual void pack_command_line_args() override;

   private:
    vector<Atom*> build_atoms_from_tokens();
    void add_atoms();

    string action;
    vector<string> tokens;
    mutex api_mutex;
    shared_ptr<AtomDB> atomdb;
};

}  // namespace atomdb_broker
