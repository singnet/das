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

static void redis_argv(const vector<string>& args, vector<const char*>& argv, vector<size_t>& argvlen) {
    argv.clear();
    argvlen.clear();
    argv.reserve(args.size());
    argvlen.reserve(args.size());
    for (const auto& arg : args) {
        argv.push_back(arg.c_str());
        argvlen.push_back(arg.size());
    }
}

redisReply* RedisContext::execute(const vector<string>& args) {
    vector<const char*> argv;
    vector<size_t> argvlen;
    redis_argv(args, argv, argvlen);
    if (cluster_flag) {
        return (redisReply*) redisClusterCommandArgv(
            cluster_ctx, static_cast<int>(argv.size()), argv.data(), argvlen.data());
    } else {
        return (redisReply*) redisCommandArgv(
            single_ctx, static_cast<int>(argv.size()), argv.data(), argvlen.data());
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
            RAISE_ERROR("Failed to append command to Redis cluster context.");
        }
    } else {
        if (redisAppendCommand(single_ctx, command) == REDIS_OK) {
            pending_commands_count++;
        } else {
            RAISE_ERROR("Failed to append command to Redis context.");
        }
    }
}

void RedisContext::append_command(const vector<string>& args) {
    vector<const char*> argv;
    vector<size_t> argvlen;
    redis_argv(args, argv, argvlen);
    int status;
    if (cluster_flag) {
        status = redisClusterAppendCommandArgv(
            cluster_ctx, static_cast<int>(argv.size()), argv.data(), argvlen.data());
    } else {
        status = redisAppendCommandArgv(
            single_ctx, static_cast<int>(argv.size()), argv.data(), argvlen.data());
    }
    if (status == REDIS_OK) {
        pending_commands_count++;
    } else {
        RAISE_ERROR("Failed to append command to Redis context.");
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
