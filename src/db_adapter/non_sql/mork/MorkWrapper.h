#pragma once

#include <memory>
#include <string>

#include "BoundedSharedQueue.h"
#include "DatabaseTypes.h"
#include "DatabaseWrapper.h"
#include "MorkConnection.h"

using namespace std;

namespace db_adapter {

class MorkWrapper : public DatabaseWrapper {
   public:
    MorkWrapper(MorkConnection& conn,
                shared_ptr<BoundedSharedQueue> output_queue,
                MAPPER_TYPE mapper_type = MAPPER_TYPE::METTA2ATOMS);
    ~MorkWrapper();

    void map(const string& metta_query);

   private:
    MorkConnection& conn;
    shared_ptr<BoundedSharedQueue> output_queue;
};

}  // namespace db_adapter
