#include "AtomDBFactory.h"

#include "InMemoryDB.h"
#include "MorkDB.h"
#include "RedisMongoDB.h"
#include "Utils.h"

using namespace atomdb;
using namespace commons;

// --------------------------------------------------------------------------------
// Public methods

shared_ptr<AtomDB> AtomDBFactory::create(const JsonConfig& config, const string& context) {
    return wrap_if_protected(create_backend(config, context));
}

shared_ptr<AtomDB> AtomDBFactory::create_backend(const JsonConfig& config, const string& context) {
    auto atomdb_type = config.at_path("type").get_or<string>("");

    if (atomdb_type == "redismongodb") {
        return shared_ptr<AtomDB>(new RedisMongoDB(context, false, config));
    }
    if (atomdb_type == "morkdb") {
        return shared_ptr<AtomDB>(new MorkDB(context, config));
    }
    if (atomdb_type == "inmemorydb") {
        return make_shared<InMemoryDB>(context.empty() ? "inmemorydb_" : context);
    }

    RAISE_ERROR("AtomDBFactory: unsupported AtomDB type: " + atomdb_type);
    return shared_ptr<AtomDB>{};
}

shared_ptr<AtomDB> AtomDBFactory::wrap_if_protected(shared_ptr<AtomDB> backend) {
    // AtomDBFactory::wrap_if_protected() is not implemented yet.
    return backend;
}