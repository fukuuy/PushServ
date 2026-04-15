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

    std::cout << "GRPC push to " << uid << ": " << content << std::endl;

    KafkaProducer::Instance().Send(KAFKA_TOPIC, uid, content);

    if (PushServer::Instance().IsUserOnline(uid)) {
        PushServer::Instance().SendToUser(uid, content);
        rsp->set_code(0);
        rsp->set_msg("pushed");
    }
    else {
        MySQLClient::Instance().InsertOfflineMsg(uid, content);
        rsp->set_code(1);
        rsp->set_msg("offline, saved to DB");
    }
    return grpc::Status::OK;
}