#include "kafka_consumer.h"
#include "common.h"
#include "push_server.h"
#include "mysql_client.h"
#include <iostream>
#include <csignal>

KafkaConsumer& KafkaConsumer::Instance() {
    static KafkaConsumer instance;
    return instance;
}

KafkaConsumer::~KafkaConsumer() {
    Stop();
    if (consumer_) {
        consumer_->close();
        delete consumer_;
    }
}

bool KafkaConsumer::Init(const std::string& brokers, const std::string& group_id, const std::string& topic) {
    std::string err;
    RdKafka::Conf* conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    conf->set("bootstrap.servers", brokers, err);
    conf->set("group.id", group_id, err);
    conf->set("auto.offset.reset", "earliest", err);
    conf->set("enable.auto.commit", "true", err);

    consumer_ = RdKafka::KafkaConsumer::create(conf, err);
    delete conf;
    if (!consumer_) {
        std::cerr << "Failed to create Kafka consumer: " << err << std::endl;
        return false;
    }

    std::vector<std::string> topics = { topic };
    RdKafka::ErrorCode ec = consumer_->subscribe(topics);
    if (ec) {
        std::cerr << "Failed to subscribe to topic: " << RdKafka::err2str(ec) << std::endl;
        return false;
    }

    std::cout << "Kafka consumer initialized, subscribed to topic: " << topic << std::endl;
    return true;
}

void KafkaConsumer::Start() {
    if (running_) return;
    running_ = true;
    worker_ = std::thread(&KafkaConsumer::ConsumeLoop, this);
}

void KafkaConsumer::Stop() {
    running_ = false;
    if (worker_.joinable()) {
        worker_.join();
    }
}

void KafkaConsumer::ConsumeLoop() {
    while (running_) {
        RdKafka::Message* msg = consumer_->consume(100); // 100ms 

        if (msg->err() == RdKafka::ERR_NO_ERROR) {
            std::string uid = msg->key() ? *msg->key() : "";
            std::string content = msg->payload() ? std::string(static_cast<const char*>(msg->payload()), msg->len()) : "";

            std::cout << "[Consumer] Received message for user: " << uid << std::endl;

            if (!uid.empty() && !content.empty()) {
                if (PushServer::Instance().IsUserOnline(uid)) {
                    // 在线
                    PushServer::Instance().SendToUser(uid, content);
                    std::cout << "[Consumer] Pushed to online user: " << uid << std::endl;
                }
                else {
                    // 离线
                    MySQLClient::Instance().InsertOfflineMsg(uid, content);
                    std::cout << "[Consumer] Saved offline message for user: " << uid << std::endl;
                }
            }
        }
        else if (msg->err() != RdKafka::ERR__TIMED_OUT) {
            std::cerr << "Kafka consumer error: " << msg->errstr() << std::endl;
        }
        delete msg;
    }
}