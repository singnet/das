#include "RedisContextPool.h"

#include "RedisContext.h"
#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace commons;

const size_t MAX_CONTEXTS = 100;
const unsigned int MAX_RETRY_COUNT = 5;

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
        if (ctx->ping()) {
            LOG_DEBUG("Context is alive, returning to the caller.");
            return shared_ptr<RedisContext>(ctx.get(),
                                            [this, ctx](RedisContext* ptr) { this->release(ctx); });
        }
        LOG_DEBUG("Context is not alive, creating a new one.");
        total_contexts--;
    }

    // Create a new context if the pool is empty
    shared_ptr<RedisContext> ctx = make_shared<RedisContext>(cluster_flag);

    unsigned int retry_delay = 1000;
    unsigned int retries = 1;
    while (retries <= MAX_RETRY_COUNT) {
        try {
            if (cluster_flag) {
                auto cluster_ctx = redisClusterConnect(cluster_address.c_str(), 0);
                if (!cluster_ctx || cluster_ctx->err) {
                    string err = cluster_ctx ? cluster_ctx->errstr : "Unknown error";
                    if (cluster_ctx) redisClusterFree(cluster_ctx);
                    throw runtime_error("Redis cluster connection error (" + to_string(retries) + " / " +
                                        to_string(MAX_RETRY_COUNT) + "): " + err);
                }
                ctx->set_context(cluster_ctx);
            } else {
                auto single_ctx = redisConnect(host.c_str(), stoi(port));
                if (!single_ctx || single_ctx->err) {
                    string err = single_ctx ? single_ctx->errstr : "Unknown error";
                    if (single_ctx) redisFree(single_ctx);
                    throw runtime_error("Redis connection error (" + to_string(retries) + " / " +
                                        to_string(MAX_RETRY_COUNT) + "): " + err);
                }
                ctx->set_context(single_ctx);
            }
            break;
        } catch (const std::exception& e) {
            retries++;
            LOG_ERROR(string(e.what()));
            if (retries <= MAX_RETRY_COUNT) Utils::sleep(retry_delay);
            retry_delay *= 2;
        }
    }

    if (retries > MAX_RETRY_COUNT) {
        Utils::error("Redis connection error: Failed to connect to Redis after " +
                     to_string(MAX_RETRY_COUNT) + " retries");
    }

    total_contexts++;

    // Deleter calls release() to put it back in the pool
    return shared_ptr<RedisContext>(ctx.get(), [this, ctx](RedisContext* ptr) { this->release(ctx); });
}

void RedisContextPool::release(shared_ptr<RedisContext> ctx) {
    if (!ctx) return;
    unique_lock<mutex> lock(this->pool_mutex);
    pool.push(ctx);
    LOG_DEBUG("Context added to the pool (size: " << pool.size()
                                                  << ", contexts: " << to_string(total_contexts) << ")");
    pool_cond_var.notify_all();
}
