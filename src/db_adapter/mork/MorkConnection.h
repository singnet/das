#pragma once

#include <memory>
#include <string>
#include <vector>

#include "DatabaseConnection.h"
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

}  // namespace db_adapter
