#pragma once
#include <string>
#include <librdkafka/rdkafkacpp.h>

class KafkaProducer {
public:
    static KafkaProducer& Instance();
    bool Init(const std::string& brokers);
    void Send(const std::string& topic, const std::string& key, const std::string& msg);

private:
    KafkaProducer() = default;
    RdKafka::Producer* producer_ = nullptr;
};