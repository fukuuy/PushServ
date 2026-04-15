#include "kafka_client.h"
#include <iostream>

KafkaProducer& KafkaProducer::Instance() {
    static KafkaProducer ins;
    return ins;
}

bool KafkaProducer::Init(const std::string& brokers) {
    std::string err;
    RdKafka::Conf* conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    conf->set("metadata.broker.list", brokers, err);
    producer_ = RdKafka::Producer::create(conf, err);
    delete conf;
    return producer_ != nullptr;
}

void KafkaProducer::Send(const std::string& topic, const std::string& key, const std::string& msg) {
    if (!producer_) return;

    producer_->produce(
        topic,
        RdKafka::Topic::PARTITION_UA,
        RdKafka::Producer::RK_MSG_COPY,
        (void*)msg.data(),
        msg.size(),
        key.data(),
        key.size(),
        0,
        nullptr,
        nullptr
    );
    producer_->poll(0);
}