#include "link_creation_console.h"

#include "AtomDBSingleton.h"
#include "Logger.h"

using namespace link_creation_agent;
using namespace std;
using namespace atomdb;

shared_ptr<Console> Console::console_instance = nullptr;

shared_ptr<Console> Console::get_instance() {
    if (console_instance == nullptr) {
        console_instance = shared_ptr<Console>(new Console());
        try {
            AtomDBSingleton::init();
        } catch (const exception& e) {
        }
    }
    return console_instance;
}

LinkTargetTypes Console::get_atom(string handle) {
    shared_ptr<AtomDB> atom_db = AtomDBSingleton::get_instance();
    auto atom = atom_db->get_atom_document(handle.c_str());
    if (atom->contains("name")) {
        Node node;
        node.type = atom->get("named_type");
        node.value = atom->get("name");
        return node;
    }
    if (atom->contains("targets")) {
        shared_ptr<Link> link = make_shared<Link>();
        link->set_type(atom->get("named_type"));
        auto array_size = atom->get_size("targets");
        for (int i = 0; i < array_size; i++) {
            auto target = atom->get("targets", i);
            link->add_target(get_atom(target));
        }
        return link;
    }
    Utils::error("Error parsing Atom: " + handle);
}

string Console::tokens_to_metta_string(vector<string> tokens, bool has_custom_field_size) {
    try {
        if (tokens.front() == "LINK") {
            vector<string> link_tokens;
            for (int i = 0; i < tokens.size(); i++) {
                if (tokens[i] == "HANDLE") {
                    i++;
                    auto atom = get_atom(tokens[i]);
                    if (holds_alternative<shared_ptr<Link>>(atom)) {
                        for (auto token : get<shared_ptr<Link>>(atom)->tokenize()) {
                            link_tokens.push_back(token);
                        }
                    }
                    if (holds_alternative<Node>(atom)) {
                        for (auto token : get<Node>(atom).tokenize()) {
                            link_tokens.push_back(token);
                        }
                    }
                } else {
                    link_tokens.push_back(tokens[i]);
                }
            }
            Link link = Link::untokenize(link_tokens, has_custom_field_size);
            string metta_string = link.to_metta_string();
            return metta_string;
        }
        if (tokens.front() == "NODE") {
            string metta_string = "(: " + tokens[2] + " " + tokens[1] + ")";
            return metta_string;
        }
        LOG_ERROR("Failed to create MeTTa expression for " << Utils::join(tokens, ' '));
        return "";
    } catch (const exception& e) {
        LOG_ERROR("Exception: " << e.what());
        LOG_ERROR("Failed to create MeTTa expression for " << Utils::join(tokens, ' '));
        return "";
    }
}

void Console::print_metta(vector<string> tokens, bool has_custom_field_size) {
    LOG_INFO("MeTTa Expression: " << tokens_to_metta_string(tokens, has_custom_field_size));
}