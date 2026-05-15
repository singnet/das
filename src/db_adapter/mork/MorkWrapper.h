#pragma once

#include <memory>
#include <string>
#include <vector>

#include "BoundedSharedQueue.h"
#include "DatabaseWrapper.h"
#include "MorkDB.h"

using namespace std;
using namespace atomdb;

namespace db_adapter {

class MorkConnection : public DatabaseConnection {
   public:
    MorkConnection(const string& id, const string& host, int port);
    ~MorkConnection();

    void connect() override;
    void disconnect() override;

    vector<string> query(const string& metta_query);

   protected:
    unique_ptr<MorkClient> conn;
};

class MorkWrapper : public DatabaseWrapper {
   public:
    MorkWrapper(MorkConnection& conn,
                shared_ptr<BoundedSharedQueue> output_queue,
                MAPPER_TYPE mapper_type = MAPPER_TYPE::MORK2ATOMS);
    ~MorkWrapper();

    void map(const string& metta_query);

   private:
    MorkConnection& conn;
    shared_ptr<BoundedSharedQueue> output_queue;
};

}  // namespace db_adapter
