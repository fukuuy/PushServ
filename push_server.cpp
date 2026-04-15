#include "push_server.h"
#include <iostream>

PushServer& PushServer::Instance() {
    static PushServer ins;
    return ins;
}

bool PushServer::Start(int port) {
    base_ = event_base_new();
    if (!base_) return false;

    sockaddr_in sin{};
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);

    auto listener = evconnlistener_new_bind(
        base_, OnAccept, nullptr,
        LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
        1024, (sockaddr*)&sin, sizeof(sin));

    if (!listener) return false;

    std::cout << "Push server started on 0.0.0.0:" << port << std::endl;

    event* timer = event_new(base_, -1, EV_PERSIST, HeartbeatTimer, this);
    timeval tv{ 1, 0 };
    event_add(timer, &tv);

    event_base_dispatch(base_);
    return true;
}

void PushServer::OnAccept(evconnlistener*, evutil_socket_t fd, sockaddr*, int, void*) {
    auto& server = Instance();
    auto bev = bufferevent_socket_new(server.base_, fd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, OnRead, nullptr, OnEvent, &server);
    bufferevent_enable(bev, EV_READ);

    std::lock_guard<std::mutex> lock(server.mutex_);
    server.conn_map_[bev] = { bev, "", time(nullptr) };
}

void PushServer::OnRead(bufferevent* bev, void* ctx) {
    auto& server = *static_cast<PushServer*>(ctx);
    char buf[1024];
    auto n = bufferevent_read(bev, buf, sizeof(buf) - 1);
    if (n <= 0) return;
    buf[n] = 0;
    std::string data(buf);

    std::lock_guard<std::mutex> lock(server.mutex_);
    auto it = server.conn_map_.find(bev);
    if (it == server.conn_map_.end()) return;

    auto& conn = it->second;
    conn.last_heartbeat = time(nullptr);

    if (data.find("uid=") == 0) {
        std::string uid = data.substr(4);
        conn.user_id = uid;
        server.user_map_[uid] = bev;
        RedisClient::Instance().SetUserOnline(uid, "127.0.0.1:8888");
        std::cout << "User login: " << uid << std::endl;
    }
}

void PushServer::OnEvent(bufferevent* bev, short events, void* ctx) {
    auto& server = *static_cast<PushServer*>(ctx);
    if (events & (BEV_EVENT_ERROR | BEV_EVENT_EOF)) {
        server.RemoveClient(bev);
    }
}

void PushServer::RemoveClient(bufferevent* bev) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = conn_map_.find(bev);
    if (it == conn_map_.end()) return;

    std::string uid = it->second.user_id;
    if (!uid.empty()) {
        user_map_.erase(uid);
        RedisClient::Instance().SetUserOffline(uid);
    }
    conn_map_.erase(it);
    bufferevent_free(bev);
}

void PushServer::HeartbeatTimer(evutil_socket_t, short, void* arg) {
    auto& server = *static_cast<PushServer*>(arg);
    std::lock_guard<std::mutex> lock(server.mutex_);

    auto now = time(nullptr);
    for (auto it = server.conn_map_.begin(); it != server.conn_map_.end();) {
        auto& conn = it->second;
        if (now - conn.last_heartbeat > HEARTBEAT_TIMEOUT) {
            std::string uid = conn.user_id;
            server.user_map_.erase(uid);
            RedisClient::Instance().SetUserOffline(uid);
            bufferevent_free(conn.bev);
            it = server.conn_map_.erase(it);
        }
        else {
            ++it;
        }
    }
}

bool PushServer::IsUserOnline(const std::string& uid) {
    std::lock_guard<std::mutex> lock(mutex_);
    return user_map_.count(uid) > 0;
}

void PushServer::SendToUser(const std::string& uid, const std::string& content) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = user_map_.find(uid);
    if (it != user_map_.end()) {
        bufferevent_write(it->second, content.c_str(), content.size());
    }
}