#pragma once
#include <string>
#include <hiredis/hiredis.h>

class RedisClient {
public:
    static RedisClient& Instance();
    bool Connect(const std::string& ip, int port);
    void SetUserOnline(const std::string& uid, const std::string& server_addr);
    void SetUserOffline(const std::string& uid);
    std::string GetUserServer(const std::string& uid);

private:
    RedisClient() = default;
    redisContext* ctx_ = nullptr;
};
