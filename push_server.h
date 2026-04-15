#pragma once
#include "common.h"
#include "redis_client.h"

class PushServer {
public:
    static PushServer& Instance();
    bool Start(int port);
    void SendToUser(const std::string& uid, const std::string& content);
    bool IsUserOnline(const std::string& uid);

private:
    PushServer() = default;

    static void OnAccept(evconnlistener*, evutil_socket_t, sockaddr*, int, void*);
    static void OnRead(bufferevent* bev, void* ctx);
    static void OnEvent(bufferevent* bev, short events, void* ctx);
    static void HeartbeatTimer(evutil_socket_t, short, void*);

    void RemoveClient(bufferevent* bev);

    event_base* base_ = nullptr;
    std::unordered_map<bufferevent*, ClientConn> conn_map_;
    std::unordered_map<std::string, bufferevent*> user_map_;
    std::mutex mutex_;
};
