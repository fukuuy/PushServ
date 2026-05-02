#include "push_server.h"
#include "push_grpc.h"
#include "redis_client.h"
#include "kafka_client.h"
#include "kafka_consumer.h"
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
    // 初始化依赖
    if (!RedisClient::Instance().Connect(REDIS_HOST, REDIS_PORT)) {
        printf("redis connect failed\n");
        return 1;
    }
    if (!KafkaProducer::Instance().Init(KAFKA_BROKER)) {
        printf("kafka producer init failed\n");
        return 1;
    }
    if (!MySQLClient::Instance().Connect()) {
        printf("mysql connect failed\n");
        return 1;
    }

    // 初始化 Kafka 消费者
    if (!KafkaConsumer::Instance().Init(KAFKA_BROKER, KAFKA_GROUP_ID, KAFKA_TOPIC)) {
        printf("kafka consumer init failed\n");
        return 1;
    }
    KafkaConsumer::Instance().Start();

    // 启动 gRPC 服务线程
    std::thread grpc_thread(RunGRPC);

    // 启动 TCP 推送服务
    PushServer::Instance().Start(SERVER_PORT);

    
    KafkaConsumer::Instance().Stop();
    grpc_thread.join();
    return 0;
}
