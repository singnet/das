#include "atomdb_operations.h"

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

void AddAtom::add_node() {
    run_benchmark(
        "add_node",
        [&](int i) -> Node* {
            return new Node("Symbol", "\"NODE_t" + to_string(tid_) + "_i" + to_string(i) + "\"");
        },
        [&](Node* node) { db_->add_node(node); });
}
void AddAtom::add_link() {
    run_benchmark(
        "add_link",
        [&](int i) -> Link* {
            string suffix = to_string(tid_) + "_i" + to_string(i);
            auto node_equivalence = new Node("Symbol", "EQUIVALENCE_t" + suffix);
            auto node_equivalence_handle = db_->add_node(node_equivalence);
            auto node_a = new Node("Symbol", "NODE_A_t" + suffix);
            auto node_a_handle = db_->add_node(node_a);
            auto node_b = new Node("Symbol", "NODE_B_t" + suffix);
            auto node_b_handle = db_->add_node(node_b);
            return new Link("Expression", {node_equivalence_handle, node_a_handle, node_b_handle});
        },
        [&](Link* link) { db_->add_link(link); });
}
void AddAtom::add_atom_node() {
    run_benchmark(
        "add_atom[node]",
        [&](int i) -> Atom* {
            return new Node("Symbol", "\"NODE_t" + to_string(tid_) + "_i" + to_string(i) + "\"");
        },
        [&](Atom* atom) { db_->add_atom(atom); });
}
void AddAtom::add_atom_link() {
    run_benchmark(
        "add_atom[link]",
        [&](int i) -> Atom* {
            string suffix = to_string(tid_) + "_i" + to_string(i);
            auto node_equivalence = new Node("Symbol", "EQUIVALENCE_t" + suffix);
            auto node_equivalence_handle = db_->add_node(node_equivalence);
            auto node_a = new Node("Symbol", "\"NODE_A_t" + suffix + "\"");
            auto node_a_handle = db_->add_node(node_a);
            auto node_b = new Node("Symbol", "\"NODE_B_t" + suffix + "\"");
            auto node_b_handle = db_->add_node(node_b);
            return new Link("Expression", {node_equivalence_handle, node_a_handle, node_b_handle});
        },
        [&](Atom* atom) { db_->add_atom(atom); });
}

void AddAtoms::add_nodes() {
    run_benchmark(
        "add_nodes",
        [&](int i) -> vector<Node*> {
            vector<Node*> nodes;
            for (size_t j = 0; j < BATCH_SIZE; j++) {
                nodes.push_back(new Node(
                    "Symbol",
                    "\"NODES_A_t" + to_string(tid_) + "_i" + to_string(i) + "_j" + to_string(j) + "\""));
            }
            return nodes;
        },
        [&](vector<Node*> nodes) { db_->add_nodes(nodes); },
        BATCH_SIZE);
}
void AddAtoms::add_links() {
    run_benchmark(
        "add_links",
        [&](int i) -> vector<Link*> {
            vector<Link*> links;
            for (size_t j = 0; j < BATCH_SIZE; j++) {
                string suffix = to_string(tid_) + "_i" + to_string(i) + "_j" + to_string(j);
                auto node_equivalence = new Node("Symbol", "EQUIVALENCE_A_t" + suffix);
                auto node_equivalence_handle = db_->add_node(node_equivalence);
                auto node_b = new Node("Symbol", "\"NODES_B_t" + suffix + "\"");
                auto node_b_handle = db_->add_node(node_b);
                auto node_c = new Node("Symbol", "\"NODES_C_t" + suffix + "\"");
                auto node_c_handle = db_->add_node(node_c);
                links.push_back(
                    new Link("Expression", {node_equivalence_handle, node_b_handle, node_c_handle}));
            }
            return links;
        },
        [&](vector<Link*> links) { db_->add_links(links); },
        BATCH_SIZE);
}
void AddAtoms::add_atoms_node() {
    run_benchmark(
        "add_atoms[node]",
        [&](int i) -> vector<Atom*> {
            vector<Atom*> atoms;
            for (size_t j = 0; j < BATCH_SIZE; j++) {
                atoms.push_back(new Node(
                    "Symbol",
                    "\"NODES_A_t" + to_string(tid_) + "_i" + to_string(i) + "_j" + to_string(j) + "\""));
            }
            return atoms;
        },
        [&](vector<Atom*> atoms) { db_->add_atoms(atoms); },
        BATCH_SIZE);
}
void AddAtoms::add_atoms_link() {
    run_benchmark(
        "add_atoms[link]",
        [&](int i) -> vector<Atom*> {
            vector<Atom*> atoms;
            for (size_t j = 0; j < BATCH_SIZE; j++) {
                string suffix = to_string(tid_) + "_i" + to_string(i) + "_j" + to_string(j);
                auto node_equivalence = new Node("Symbol", "EQUIVALENCE_A_t" + suffix);
                auto node_equivalence_handle = db_->add_node(node_equivalence);
                auto node_b = new Node("Symbol", "\"NODES_B_t" + suffix + "\"");
                auto node_b_handle = db_->add_node(node_b);
                auto node_c = new Node("Symbol", "\"NODES_C_t" + suffix + "\"");
                auto node_c_handle = db_->add_node(node_c);
                atoms.push_back(
                    new Link("Expression", {node_equivalence_handle, node_b_handle, node_c_handle}));
            }
            return atoms;
        },
        [&](vector<Atom*> atoms) { db_->add_atoms(atoms); },
        BATCH_SIZE);
}

void GetAtom::get_node_document() {
    run_benchmark(
        "get_node_document",
        [&](int i) -> string {
            auto link_schema = LinkSchema(contains_links_query);
            auto handle_set = db_->query_for_pattern(link_schema);
            vector<string> random_link_handles = get_random_link_handle(handle_set);
            auto link_document = db_->get_link_document(random_link_handles[0]);
            auto contains_node_handle = string(link_document->get("targets", 0));
            return contains_node_handle;
        },
        [&](string handle) { db_->get_node_document(handle); });
}
void GetAtom::get_link_document() {
    run_benchmark(
        "get_link_document",
        [&](int i) -> string {
            auto link_schema = LinkSchema(contains_links_query);
            auto handle_set = db_->query_for_pattern(link_schema);
            vector<string> random_link_handles = get_random_link_handle(handle_set);
            return random_link_handles[0];
        },
        [&](string handle) { db_->get_link_document(handle); });
}
void GetAtom::get_atom_document_node() {
    run_benchmark(
        "get_atom_document[node]",
        [&](int i) -> string {
            auto link_schema = LinkSchema(contains_links_query);
            auto handle_set = db_->query_for_pattern(link_schema);
            vector<string> random_link_handles = get_random_link_handle(handle_set);
            auto link_document = db_->get_link_document(random_link_handles[0]);
            auto contains_node_handle = string(link_document->get("targets", 0));
            return contains_node_handle;
        },
        [&](string handle) { db_->get_atom_document(handle); });
}
void GetAtom::get_atom_document_link() {
    run_benchmark(
        "get_atom_document[link]",
        [&](int i) -> string {
            auto link_schema = LinkSchema(contains_links_query);
            auto handle_set = db_->query_for_pattern(link_schema);
            vector<string> random_link_handles = get_random_link_handle(handle_set);
            return random_link_handles[0];
        },
        [&](string handle) { db_->get_atom_document(handle); });
}
void GetAtom::get_atom_node() {
    run_benchmark(
        "get_atom[node]",
        [&](int i) -> string {
            auto link_schema = LinkSchema(contains_links_query);
            auto handle_set = db_->query_for_pattern(link_schema);
            vector<string> random_link_handles = get_random_link_handle(handle_set);
            auto link_document = db_->get_link_document(random_link_handles[0]);
            auto contains_node_handle = string(link_document->get("targets", 0));
            return contains_node_handle;
        },
        [&](string handle) { db_->get_atom(handle); });
}
void GetAtom::get_atom_link() {
    run_benchmark(
        "get_atom[link]",
        [&](int i) -> string {
            auto link_schema = LinkSchema(contains_links_query);
            auto handle_set = db_->query_for_pattern(link_schema);
            vector<string> random_link_handles = get_random_link_handle(handle_set);
            return random_link_handles[0];
        },
        [&](string handle) { db_->get_atom(handle); });
}

void GetAtoms::get_node_documents() {
    run_benchmark(
        "get_node_documents",
        [&](int i) -> vector<string> {
            auto link_schema = LinkSchema(sentence_links_query);
            auto handle_set = db_->query_for_pattern(link_schema);
            size_t max_count = max<size_t>(BATCH_SIZE, MAX_COUNT);
            vector<string> random_link_handles =
                get_random_link_handle(handle_set, max_count, BATCH_SIZE);

            vector<string> handles;
            for (const auto& link_handle : random_link_handles) {
                auto link_document = db_->get_link_document(link_handle);
                auto node_handle = string(link_document->get("targets", 1));
                handles.push_back(node_handle);
            }
            return handles;
        },
        [&](vector<string> handles) { db_->get_node_documents(handles, {"_id"}); });
}
void GetAtoms::get_link_documents() {
    run_benchmark(
        "get_link_documents",
        [&](int i) -> vector<string> {
            auto link_schema = LinkSchema(sentence_links_query);
            auto handle_set = db_->query_for_pattern(link_schema);
            size_t max_count = max<size_t>(BATCH_SIZE, MAX_COUNT);
            return get_random_link_handle(handle_set, max_count, BATCH_SIZE);
        },
        [&](vector<string> handles) { db_->get_link_documents(handles, {"_id"}); });
}
void GetAtoms::get_atom_documents_node() {
    run_benchmark(
        "get_atom_documents[node]",
        [&](int i) -> vector<string> {
            auto link_schema = LinkSchema(sentence_links_query);
            auto handle_set = db_->query_for_pattern(link_schema);
            size_t max_count = max<size_t>(BATCH_SIZE, MAX_COUNT);
            vector<string> random_link_handles =
                get_random_link_handle(handle_set, max_count, BATCH_SIZE);

            vector<string> handles;
            for (const auto& link_handle : random_link_handles) {
                auto link_document = db_->get_link_document(link_handle);
                auto node_handle = string(link_document->get("targets", 1));
                handles.push_back(node_handle);
            }
            return handles;
        },
        [&](vector<string> handles) { db_->get_atom_documents(handles, {"_id"}); });
}
void GetAtoms::get_atom_documents_link() {
    run_benchmark(
        "get_atom_documents[link]",
        [&](int i) -> vector<string> {
            auto link_schema = LinkSchema(sentence_links_query);
            auto handle_set = db_->query_for_pattern(link_schema);
            size_t max_count = max<size_t>(BATCH_SIZE, MAX_COUNT);
            return get_random_link_handle(handle_set, max_count, BATCH_SIZE);
        },
        [&](vector<string> handles) { db_->get_atom_documents(handles, {"_id"}); });
}
void GetAtoms::query_for_pattern() {
    run_benchmark(
        "query_for_pattern[first_result]",
        [&](int i) -> vector<string> { return contains_links_query; },
        [&](vector<string> pattern) {
            auto handles_set = db_->query_for_pattern(pattern);
            auto iterator = handles_set->get_iterator();
            iterator->next();
        });
}
void GetAtoms::query_for_targets() {
    run_benchmark(
        "query_for_targets",
        [&](int i) -> string {
            auto link_schema = LinkSchema(sentence_links_query);
            auto handle_set = db_->query_for_pattern(link_schema);
            vector<string> random_link_handles = get_random_link_handle(handle_set);
            return random_link_handles[0];
        },
        [&](string handle) { db_->query_for_targets(handle); });
}

void DeleteAtom::delete_node(string type) {
    run_benchmark(
        "delete_node",
        [&](int i) -> string {
            auto link_schema = LinkSchema(sentence_links_query);
            shared_ptr<atomdb_api_types::HandleSet> handle_set;
            if (type == "mork") {
                handle_set = dynamic_pointer_cast<MorkDB>(db_)->query_for_pattern_base(link_schema);
            } else {
                handle_set = db_->query_for_pattern(link_schema);
            }
            vector<string> random_link_handles = get_random_link_handle(handle_set);
            auto link_document = db_->get_link_document(random_link_handles[0]);
            auto node_handle = string(link_document->get("targets", 1));
            return node_handle;
        },
        [&](string handle) { db_->delete_node(handle); });
}
void DeleteAtom::delete_link(string type) {
    run_benchmark(
        "delete_link",
        [&](int i) -> string {
            auto link_schema = LinkSchema(contains_links_query);
            shared_ptr<atomdb_api_types::HandleSet> handle_set;
            if (type == "mork") {
                handle_set = dynamic_pointer_cast<MorkDB>(db_)->query_for_pattern_base(link_schema);
            } else {
                handle_set = db_->query_for_pattern(link_schema);
            }
            vector<string> random_link_handles = get_random_link_handle(handle_set);
            return random_link_handles[0];
        },
        [&](string handle) { db_->delete_link(handle); });
}
void DeleteAtom::delete_atom_node(string type) {
    run_benchmark(
        "delete_atom[node]",
        [&](int i) -> string {
            auto link_schema = LinkSchema(sentence_links_query);
            shared_ptr<atomdb_api_types::HandleSet> handle_set;
            if (type == "mork") {
                handle_set = dynamic_pointer_cast<MorkDB>(db_)->query_for_pattern_base(link_schema);
            } else {
                handle_set = db_->query_for_pattern(link_schema);
            }
            vector<string> random_link_handles = get_random_link_handle(handle_set);
            auto link_document = db_->get_link_document(random_link_handles[0]);
            auto node_handle = string(link_document->get("targets", 1));
            return node_handle;
        },
        [&](string handle) { db_->delete_atom(handle); });
}
void DeleteAtom::delete_atom_link(string type) {
    run_benchmark(
        "delete_atom[link]",
        [&](int i) -> string {
            auto link_schema = LinkSchema(contains_links_query);
            shared_ptr<atomdb_api_types::HandleSet> handle_set;
            if (type == "mork") {
                handle_set = dynamic_pointer_cast<MorkDB>(db_)->query_for_pattern_base(link_schema);
            } else {
                handle_set = db_->query_for_pattern(link_schema);
            }
            vector<string> random_link_handles = get_random_link_handle(handle_set);
            return random_link_handles[0];
        },
        [&](string handle) { db_->delete_atom(handle); });
}

void DeleteAtoms::delete_nodes(string type) {
    run_benchmark(
        "delete_nodes",
        [&](int i) -> vector<string> {
            auto link_schema = LinkSchema(sentence_links_query);
            shared_ptr<atomdb_api_types::HandleSet> handle_set;
            if (type == "mork") {
                handle_set = dynamic_pointer_cast<MorkDB>(db_)->query_for_pattern_base(link_schema);
            } else {
                handle_set = db_->query_for_pattern(link_schema);
            }
            size_t max_count = max<size_t>(BATCH_SIZE, MAX_COUNT);
            vector<string> random_link_handles =
                get_random_link_handle(handle_set, max_count, BATCH_SIZE);

            vector<string> handles;
            for (const auto& link_handle : random_link_handles) {
                auto link_document = db_->get_link_document(link_handle);
                auto node_handle = string(link_document->get("targets", 1));
                handles.push_back(node_handle);
            }
            return handles;
        },
        [&](vector<string> handles) { db_->delete_nodes(handles); },
        BATCH_SIZE);
}
void DeleteAtoms::delete_links(string type) {
    run_benchmark(
        "delete_links",
        [&](int i) -> vector<string> {
            auto link_schema = LinkSchema(contains_links_query);
            shared_ptr<atomdb_api_types::HandleSet> handle_set;
            if (type == "mork") {
                handle_set = dynamic_pointer_cast<MorkDB>(db_)->query_for_pattern_base(link_schema);
            } else {
                handle_set = db_->query_for_pattern(link_schema);
            }
            size_t max_count = max<size_t>(BATCH_SIZE, MAX_COUNT);
            return get_random_link_handle(handle_set, max_count, BATCH_SIZE);
        },
        [&](vector<string> handles) { db_->delete_links(handles); },
        BATCH_SIZE);
}
void DeleteAtoms::delete_atoms_node(string type) {
    run_benchmark(
        "delete_atoms[node]",
        [&](int i) -> vector<string> {
            auto link_schema = LinkSchema(sentence_links_query);
            shared_ptr<atomdb_api_types::HandleSet> handle_set;
            if (type == "mork") {
                handle_set = dynamic_pointer_cast<MorkDB>(db_)->query_for_pattern_base(link_schema);
            } else {
                handle_set = db_->query_for_pattern(link_schema);
            }
            size_t max_count = max<size_t>(BATCH_SIZE, MAX_COUNT);
            vector<string> random_link_handles =
                get_random_link_handle(handle_set, max_count, BATCH_SIZE);

            vector<string> handles;
            for (const auto& link_handle : random_link_handles) {
                auto link_document = db_->get_link_document(link_handle);
                auto node_handle = string(link_document->get("targets", 1));
                handles.push_back(node_handle);
            }
            return handles;
        },
        [&](vector<string> handles) { db_->delete_atoms(handles); },
        BATCH_SIZE);
}
void DeleteAtoms::delete_atoms_link(string type) {
    run_benchmark(
        "delete_atoms[link]",
        [&](int i) -> vector<string> {
            auto link_schema = LinkSchema(contains_links_query);
            shared_ptr<atomdb_api_types::HandleSet> handle_set;
            if (type == "mork") {
                handle_set = dynamic_pointer_cast<MorkDB>(db_)->query_for_pattern_base(link_schema);
            } else {
                handle_set = db_->query_for_pattern(link_schema);
            }
            size_t max_count = max<size_t>(BATCH_SIZE, MAX_COUNT);
            return get_random_link_handle(handle_set, max_count, BATCH_SIZE);
        },
        [&](vector<string> handles) { db_->delete_atoms(handles); },
        BATCH_SIZE);
}
