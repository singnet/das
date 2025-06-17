#include "RedisContextPool.h"

#include "RedisContext.h"
#include "Utils.h"

using namespace commons;

const size_t MAX_CONTEXTS = 100;

RedisContextPool::RedisContextPool(bool is_cluster, string host, string port, string cluster_address)
    : cluster_flag(is_cluster), host(host), port(port), cluster_address(cluster_address) {
    this->pool = queue<shared_ptr<RedisContext>>();
    this->total_contexts = 0;
}

RedisContextPool::~RedisContextPool() {
    unique_lock<mutex> lock(this->pool_mutex);
    while (!pool.empty()) {
        pool.pop();
    }
}

shared_ptr<RedisContext> RedisContextPool::acquire() {
    unique_lock<mutex> lock(this->pool_mutex);
    this->pool_cond_var.wait(lock, [this]() { return !pool.empty() || total_contexts < MAX_CONTEXTS; });

    // Reuse an already existing context from the pool
    if (!pool.empty()) {
        auto ctx = pool.front();
        pool.pop();
        return ctx;
    }

    // Create a new context if the pool is empty
    shared_ptr<RedisContext> ctx = make_shared<RedisContext>(cluster_flag);
    if (cluster_flag) {
        auto cluster_ctx = redisClusterConnect(cluster_address.c_str(), 0);
        if (!cluster_ctx || cluster_ctx->err) {
            string err = cluster_ctx ? cluster_ctx->errstr : "Unknown error";
            if (cluster_ctx) redisClusterFree(cluster_ctx);
            Utils::error("Redis cluster connection error: " + err);
        }
        ctx->setContext(cluster_ctx);
    } else {
        auto single_ctx = redisConnect(host.c_str(), stoi(port));
        if (!single_ctx || single_ctx->err) {
            string err = single_ctx ? single_ctx->errstr : "Unknown error";
            if (single_ctx) redisFree(single_ctx);
            Utils::error("Redis connection error: " + err);
        }
        ctx->setContext(single_ctx);
    }

    total_contexts++;

    // Deleter calls release() to put it back in the pool
    return shared_ptr<RedisContext>(ctx.get(), [this, ctx](RedisContext* ptr) { this->release(ctx); });
}

void RedisContextPool::release(shared_ptr<RedisContext> ctx) {
    if (!ctx) return;

    unique_lock<mutex> lock(this->pool_mutex);
    pool.push(ctx);
    total_contexts--;
    pool_cond_var.notify_one();
}
