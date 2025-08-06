#pragma once

#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "atomdb_runner.h"

extern const size_t BATCH_SIZE;

class AddAtom : public AtomDBRunner {
   public:
    using AtomDBRunner::AtomDBRunner;

    void add_node();
    void add_link();
    void add_atom_node();
    void add_atom_link();
};

class AddAtoms : public AtomDBRunner {
   public:
    using AtomDBRunner::AtomDBRunner;

    void add_nodes();
    void add_links();
    void add_atoms_node();
    void add_atoms_link();
};

class GetAtom : public AtomDBRunner {
   public:
    using AtomDBRunner::AtomDBRunner;

    void get_node_document();
    void get_link_document();
    void get_atom_document_node();
    void get_atom_document_link();
    void get_atom_node();
    void get_atom_link();
};

class GetAtoms : public AtomDBRunner {
   public:
    using AtomDBRunner::AtomDBRunner;

    void get_node_documents();
    void get_link_documents();
    void get_atom_documents_node();
    void get_atom_documents_link();
    void query_for_pattern();
    void query_for_targets();
};

class DeleteAtom : public AtomDBRunner {
   public:
    using AtomDBRunner::AtomDBRunner;

    void delete_node(string type);
    void delete_link(string type);
    void delete_atom_node(string type);
    void delete_atom_link(string type);
};

class DeleteAtoms : public AtomDBRunner {
   public:
    using AtomDBRunner::AtomDBRunner;

    void delete_nodes(string type);
    void delete_links(string type);
    void delete_atoms_node(string type);
    void delete_atoms_link(string type);
};