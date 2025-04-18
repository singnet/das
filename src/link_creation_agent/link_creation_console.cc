#include "link_creation_console.h"

#include "AtomDBSingleton.h"

using namespace link_creation_agent;
using namespace std;
using namespace atomdb;

shared_ptr<Console> Console::console_instance = nullptr;

shared_ptr<Console> Console::get_instance() {
    if (!console_instance) {
        console_instance = shared_ptr<Console>(new Console());
        AtomDBSingleton::init();
    }
    return console_instance;
}

void Console::print_metta(std::vector<string> tokens) {
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
            std::cout << "Creating link" << Link().untokenize(link_tokens).to_metta_string()
                      << std::endl;
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }
}