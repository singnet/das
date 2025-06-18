#include "RedisContext.h"

RedisContext::RedisContext(bool is_cluster)
    : cluster_flag(is_cluster), single_ctx(nullptr), cluster_ctx(nullptr) {}

RedisContext::~RedisContext() {
    if (cluster_flag) {
        if (cluster_ctx) {
            redisClusterFree(cluster_ctx);
        }
    } else {
        if (single_ctx) {
            redisFree(single_ctx);
        }
    }
}

void RedisContext::set_context(redisContext* ctx) {
    if (!cluster_flag) {
        single_ctx = ctx;
    }
}

void RedisContext::set_context(redisClusterContext* ctx) {
    if (cluster_flag) {
        cluster_ctx = ctx;
    }
}

redisReply* RedisContext::execute(const char* command) {
    if (cluster_flag) {
        return (redisReply*) redisClusterCommand(cluster_ctx, command);
    } else {
        return (redisReply*) redisCommand(single_ctx, command);
    }
}

bool RedisContext::has_error() const {
    if (cluster_flag) {
        return cluster_ctx && cluster_ctx->err;
    } else {
        return single_ctx && single_ctx->err;
    }
}

const char* RedisContext::get_error() const {
    if (cluster_flag) {
        return cluster_ctx ? cluster_ctx->errstr : nullptr;
    } else {
        return single_ctx ? single_ctx->errstr : nullptr;
    }
}
