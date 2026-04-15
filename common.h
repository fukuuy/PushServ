#pragma once
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <unordered_map>
#include <mutex>

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>

struct ClientConn {
    bufferevent* bev;
    std::string user_id;
    time_t last_heartbeat;
};

const int SERVER_PORT = 8888;
const int GRPC_PORT = 50051;
const int HEARTBEAT_TIMEOUT = 30;

const std::string REDIS_HOST = "127.0.0.1";
const int REDIS_PORT = 6379;

const std::string KAFKA_BROKER = "127.0.0.1:9092";
const std::string KAFKA_TOPIC = "push_msg_topic";
const std::string KAFKA_GROUP_ID = "push_consumer_group";

const std::string MYSQL_HOST = "127.0.0.1";
const std::string MYSQL_USER = "root";
const std::string MYSQL_PASS = "123456";
const std::string MYSQL_DB = "push_db";