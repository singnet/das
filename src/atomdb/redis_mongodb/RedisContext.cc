#include "RedisContext.h"

#include "Utils.h"

using namespace commons;

RedisContext::RedisContext(bool is_cluster)
    : cluster_flag(is_cluster), single_ctx(nullptr), cluster_ctx(nullptr), pending_commands_count(0) {}

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

bool RedisContext::ping() {
    redisReply* reply = this->execute("PING");
    if (reply == NULL) return false;
    if (reply->type != REDIS_REPLY_STATUS) return false;
    return string(reply->str) == string("PONG");
}

void RedisContext::append_command(const char* command) {
    if (cluster_flag) {
        if (redisClusterAppendCommand(cluster_ctx, command) == REDIS_OK) {
            pending_commands_count++;
        } else {
            Utils::error("Failed to append command to Redis cluster context.");
        }
    } else {
        if (redisAppendCommand(single_ctx, command) == REDIS_OK) {
            pending_commands_count++;
        } else {
            Utils::error("Failed to append command to Redis context.");
        }
    }
}
void RedisContext::flush_commands() {
    redisReply* reply;
    if (cluster_flag) {
        while (pending_commands_count > 0) {
            pending_commands_count--;

            if (redisClusterGetReply(cluster_ctx, (void**) &reply) == REDIS_ERR) {
                return;
            }

            if (reply != nullptr) {
                freeReplyObject(reply);
            }
        }
    } else {
        while (pending_commands_count > 0) {
            pending_commands_count--;

            if (redisGetReply(single_ctx, (void**) &reply) == REDIS_ERR) {
                return;
            }

            if (reply != nullptr) {
                freeReplyObject(reply);
            }
        }
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
