#pragma once

#include <hiredis/hiredis.h>
#include <hiredis_cluster/hircluster.h>

#include <string>

using namespace std;

// Wrapper class for Redis context to handle both single and cluster modes
class RedisContext {
   public:
    explicit RedisContext(bool is_cluster);
    ~RedisContext();

    void set_context(redisContext* ctx);
    void set_context(redisClusterContext* ctx);

    redisReply* execute(const char* command);
    bool has_error() const;
    const char* get_error() const;

   private:
    bool cluster_flag;
    redisContext* single_ctx;
    redisClusterContext* cluster_ctx;
};
