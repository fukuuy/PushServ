#include "common.h"
#include "push_grpc.h"
#include "mysql_client.h"
#include "kafka_client.h"

#include <iostream>

grpc::Status PushServiceImpl::PushMessage(grpc::ServerContext*,
    const push::PushRequest* req,
    push::PushResponse* rsp) {
    std::string uid = req->user_id();
    std::string content = req->content();

    std::cout << "GRPC received push request for user: " << uid << std::endl;

    // 将消息写入 Kafka，由消费者异步处理
    KafkaProducer::Instance().Send(KAFKA_TOPIC, uid, content);

    // 返回受理成功
    rsp->set_code(0);
    rsp->set_msg("accepted");
    return grpc::Status::OK;
}