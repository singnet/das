#include "link_creation_console.h"

#include "AtomDBSingleton.h"
#include "Logger.h"

using namespace link_creation_agent;
using namespace std;
using namespace atomdb;

shared_ptr<Console> Console::console_instance = nullptr;

shared_ptr<Console> Console::get_instance() {
    if (!console_instance) {
        console_instance = shared_ptr<Console>(new Console());
        try {
            AtomDBSingleton::init();
        } catch (const std::exception& e) {
            cerr << "Error: " << e.what() << endl;
        }
    }
    return console_instance;
}

string Console::print_metta(std::vector<string> tokens) {
    if (tokens.front() == "LINK") {
        std::vector<string> link_tokens;
        try {
            for (int i = 0; i < tokens.size(); i++) {
                if (tokens[i] == "HANDLE") {
                    i++;
                    shared_ptr<AtomDB> atom_db = AtomDBSingleton::get_instance();
                    auto atom = atom_db->get_atom_document(tokens[i].c_str());
                    string name = atom->get("name");
                    if (name.empty()) {  // Link
                    } else {             // Node
                        string type = atom->get("named_type");
                        link_tokens.push_back("NODE");
                        link_tokens.push_back(type);
                        link_tokens.push_back(name);
                    }
                } else {
                    link_tokens.push_back(tokens[i]);
                }
            }
            std::string metta_string = Link().untokenize(link_tokens).to_metta_string();
            LOG_INFO("MeTTa Expression: " << metta_string);
            return metta_string;
        } catch (const std::exception& e) {
            LOG_ERROR("Exception: " << e.what());
            LOG_ERROR("Failed to create MeTTa expression for " << Utils::join(tokens, ' '));
            return "";
        }
    }
}