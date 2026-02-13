#include "Chain.h"

using namespace query_element;

// -------------------------------------------------------------------------------------------------
// Public methods

Chain::Chain(const array<shared_ptr<QueryElement>, 1>& clauses) : Operator<1>(clauses) {
    this->id = "CHAIN";
    //initialize(clauses);
}

Chain::~Chain() { 
    graceful_shutdown(); 
}

// --------------------------------------------------------------------------------------------
// QueryElement API

void Chain::setup_buffers() {
    Operator<1>::setup_buffers();
    //this->operator_thread = new thread(&Chain::chain_operator_method, this);
}

void Chain::graceful_shutdown() {
    Operator<1>::graceful_shutdown();
    /*
    if (this->operator_thread != NULL) {
        this->operator_thread->join();
        delete this->operator_thread;
        this->operator_thread = NULL;
    }
    */
}

