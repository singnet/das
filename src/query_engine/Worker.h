#pragma once

using namespace std;

namespace query_engine {

class Worker {
   public:
    virtual bool is_work_done() = 0;
};

}  // namespace query_engine

