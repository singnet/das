#pragma once

#include "Operator.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;

namespace query_element {

/**
 *
 */
class Chain : public Operator<1> {

public:

    // --------------------------------------------------------------------------------------------
    // Constructors and destructors

    /**
     * Constructor.
     */
    Chain(const array<shared_ptr<QueryElement>, 1>& clauses);

    /**
     * Destructor.
     */
    ~Chain();

    // --------------------------------------------------------------------------------------------
    // QueryElement API

    virtual void setup_buffers();
    virtual void graceful_shutdown();

};

} // namespace query_element
