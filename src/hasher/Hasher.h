#pragma once

#include <string>
#include <vector>
#include "expression_hasher.h"

using namespace std;
namespace commons {

class Hasher {
   public:
    static string type_handle(const string& type) {
        char *h = named_type_hash((char*) type.c_str());
        string handle(h);
        free(h);
        return handle;
    }

    static string node_handle(const string& type, const string& name) {
        char *h = terminal_hash((char*) type.c_str(), (char*) name.c_str());
        string handle(h);
        free(h);
        return handle;
    }

    static string link_handle(const string& type, const vector<string> &targets) {
        unsigned int arity = targets.size();
        char *type_hash = named_type_hash((char*) type.c_str());
        char **t = new char*[arity];
        for (unsigned int i = 0; i < arity; i++) {
            t[i] = (char*) targets[i].c_str();
        }
        char *s = expression_hash(type_hash, t, arity);
        string handle(s);
        delete[] t;
        free(type_hash);
        free(s);
        return handle;
    }

    static string composite_handle(const vector<string> &elements) {
        unsigned int n = elements.size();
        char **t = new char*[n];
        for (unsigned int i = 0; i < n; i++) {
            t[i] = (char*) elements[i].c_str();
        }
        char *s = composite_hash(t, n);
        string handle(s);
        delete[] t;
        free(s);
        return handle;
    }
};

} // namespace commons
