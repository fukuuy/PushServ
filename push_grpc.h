#pragma once
#include "push.grpc.pb.h"
#include "push_server.h"

class PushServiceImpl final : public push::PushService::Service {
public:
    grpc::Status PushMessage(grpc::ServerContext* ctx,
        const push::PushRequest* req,
        push::PushResponse* rsp) override;
};
