#pragma once

#include <hiredis/hiredis.h>
#include <hiredis_cluster/hircluster.h>

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>

#include "RedisContext.h"

using namespace std;

// Redis Context Pool
class RedisContextPool {
   public:
    RedisContextPool(bool is_cluster, string host, string port, string cluster_address);
    ~RedisContextPool();

    shared_ptr<RedisContext> acquire();

   private:
    // Release a Redis Context to the pool.
    // Private and it is called from the same thread as acquire() via its deleter.
    void release(shared_ptr<RedisContext> ctx);

    mutex pool_mutex;
    condition_variable pool_cond_var;

    queue<shared_ptr<RedisContext>> pool;
    atomic<size_t> total_contexts;

    bool cluster_flag;
    string host;
    string port;
    string cluster_address;
};
