#include "push_server.h"
#include "push_grpc.h"
#include "redis_client.h"
#include "kafka_client.h"
#include "mysql_client.h"
#include <thread>
#include <iostream>
#include <grpcpp/grpcpp.h>

using grpc::Server;
using grpc::ServerBuilder;

void RunGRPC() {
    std::string addr = "0.0.0.0:" + std::to_string(GRPC_PORT);
    PushServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "gRPC server started on " << addr << std::endl;
    server->Wait();
}

int main() {
    if (!RedisClient::Instance().Connect(REDIS_HOST, REDIS_PORT)) {
        printf("redis connect failed\n");
        return 1;
    }
    if (!KafkaProducer::Instance().Init(KAFKA_BROKER)) {
        printf("kafka init failed\n");
        return 1;
    }
    if (!MySQLClient::Instance().Connect()) {
        printf("mysql connect failed\n");
        return 1;
    }

    std::thread grpc_thread(RunGRPC);

    PushServer::Instance().Start(SERVER_PORT);

    grpc_thread.join();
    return 0;
}