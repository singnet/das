#include "LinkCreationDBHelper.h"

#include "AtomDBSingleton.h"
#include "Logger.h"

using namespace link_creation_agent;
using namespace std;
using namespace atomdb;

shared_ptr<LinkCreateDBSingleton> LinkCreateDBSingleton::instance = nullptr;

shared_ptr<LinkCreateDBSingleton> LinkCreateDBSingleton::get_instance() {
    if (instance == nullptr) {
        instance = shared_ptr<LinkCreateDBSingleton>(new LinkCreateDBSingleton());
    }
    return instance;
}

LinkTargetTypes LinkCreateDBSingleton::get_atom(string handle) {
    shared_ptr<AtomDB> atom_db = AtomDBSingleton::get_instance();
    auto atom = atom_db->get_atom_document(handle.c_str());
    if (atom->contains("name")) {
        LCANode node;
        node.type = atom->get("named_type");
        node.value = atom->get("name");
        return node;
    }
    if (atom->contains("targets")) {
        shared_ptr<LCALink> link = make_shared<LCALink>();
        link->set_type(atom->get("named_type"));
        auto array_size = atom->get_size("targets");
        for (uint i = 0; i < array_size; i++) {
            auto target = atom->get("targets", i);
            link->add_target(get_atom(target));
        }
        return link;
    }
    Utils::error("Error parsing Atom: " + handle);
    return "";
}

string LinkCreateDBSingleton::tokens_to_metta_string(vector<string> tokens, bool has_custom_field_size) {
    try {
        if (tokens.front() == "LINK") {
            vector<string> link_tokens;
            for (uint i = 0; i < tokens.size(); i++) {
                if (tokens[i] == "HANDLE") {
                    i++;
                    auto atom = get_atom(tokens[i]);
                    if (holds_alternative<shared_ptr<LCALink>>(atom)) {
                        for (auto token : get<shared_ptr<LCALink>>(atom)->tokenize()) {
                            link_tokens.push_back(token);
                        }
                    }
                    if (holds_alternative<LCANode>(atom)) {
                        for (auto token : get<LCANode>(atom).tokenize()) {
                            link_tokens.push_back(token);
                        }
                    }
                } else {
                    link_tokens.push_back(tokens[i]);
                }
            }
            LCALink link = LCALink::untokenize(link_tokens, has_custom_field_size);
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

void LinkCreateDBSingleton::print_metta(vector<string> tokens, bool has_custom_field_size) {
    LOG_INFO("MeTTa Expression: " << tokens_to_metta_string(tokens, has_custom_field_size));
}