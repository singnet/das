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

    void setContext(redisContext* ctx);
    void setContext(redisClusterContext* ctx);

    redisReply* execute(const char* command);
    bool hasError() const;
    const char* getError() const;

   private:
    bool cluster_flag;
    redisContext* single_ctx;
    redisClusterContext* cluster_ctx;
};
