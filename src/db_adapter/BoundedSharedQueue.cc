#include "BoundedSharedQueue.h"

using namespace db_adapter;

BoundedSharedQueue::BoundedSharedQueue(size_t max_size) : max_size_(max_size) {}

BoundedSharedQueue::~BoundedSharedQueue() {}

void BoundedSharedQueue::enqueue(void* item) {
    unique_lock<mutex> lock(mtx_);
    not_full_.wait(lock, [this] { return queue_.size() < max_size_; });
    queue_.push(item);
}

void* BoundedSharedQueue::dequeue() {
    lock_guard<mutex> lock(mtx_);
    if (queue_.empty()) return nullptr;
    void* item = queue_.front();
    queue_.pop();
    not_full_.notify_one();
    return item;
}

bool BoundedSharedQueue::empty() {
    lock_guard<mutex> lock(mtx_);
    return queue_.empty();
}

size_t BoundedSharedQueue::size() {
    lock_guard<mutex> lock(mtx_);
    return queue_.size();
}