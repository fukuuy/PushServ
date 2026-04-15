#pragma once
#include <string>
#include <atomic>
#include <thread>
#include <librdkafka/rdkafkacpp.h>

class KafkaConsumer {
public:
    static KafkaConsumer& Instance();
    bool Init(const std::string& brokers, const std::string& group_id, const std::string& topic);
    void Start();
    void Stop();
    bool IsRunning() const { return running_; }

private:
    KafkaConsumer() = default;
    ~KafkaConsumer();
    void ConsumeLoop();

    RdKafka::KafkaConsumer* consumer_ = nullptr;
    std::atomic<bool> running_{ false };
    std::thread worker_;
};

