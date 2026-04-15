#include "redis_client.h"

RedisClient& RedisClient::Instance() {
    static RedisClient ins;
    return ins;
}

bool RedisClient::Connect(const std::string& ip, int port) {
    std::lock_guard<std::mutex> lock(mutex_);
    ctx_ = redisConnect(ip.c_str(), port);
    return ctx_ != nullptr && !ctx_->err;
}

void RedisClient::SetUserOnline(const std::string& uid, const std::string& server_addr) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return;
    redisCommand(ctx_, "HSET online_map %s %s", uid.c_str(), server_addr.c_str());
}

void RedisClient::SetUserOffline(const std::string& uid) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return;
    redisCommand(ctx_, "HDEL online_map %s", uid.c_str());
}

std::string RedisClient::GetUserServer(const std::string& uid) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return "";
    redisReply* r = (redisReply*)redisCommand(ctx_, "HGET online_map %s", uid.c_str());
    std::string s;
    if (r && r->str) s = r->str;
    freeReplyObject(r);
    return s;
}