#pragma once

#include <mutex>
#include <string>

#include "Processor.h"

using namespace std;
using namespace processor;

namespace db_adapter {

class DatabaseConnection : public Processor {
   public:
    DatabaseConnection(const string& id, const string& host, int port);
    virtual ~DatabaseConnection();

    virtual void start() override;
    virtual void stop() override;

    virtual void connect() = 0;
    virtual void disconnect() = 0;

    bool is_connected() const;

   protected:
    string host;
    int port;

   private:
    bool connected;
    mutex connection_mutex;
};

}  // namespace db_adapter