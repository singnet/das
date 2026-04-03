#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>

using namespace std;

namespace db_adapter {

class BoundedSharedQueue {
   public:
    BoundedSharedQueue(size_t max_size);
    ~BoundedSharedQueue();

    void enqueue(void* item);
    void* dequeue();
    bool empty();
    size_t size();

   private:
    queue<void*> queue_;
    mutex mtx_;
    condition_variable not_empty_;
    condition_variable not_full_;
    size_t max_size_;
};
}  // namespace db_adapter
