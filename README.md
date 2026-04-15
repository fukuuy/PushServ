# Push Server - 高性能分布式消息推送系统

## 项目简介

Push Server 是一个基于 C++ 开发的高性能、可扩展的实时消息推送系统。系统采用 **TCP 长连接 + gRPC + Kafka + Redis + MySQL** 的架构设计，支持海量并发连接、跨实例消息路由、离线消息持久化以及异步消息处理，可广泛应用于社交应用通知、IM 即时通讯、实时数据推送、系统监控告警等场景。

### 核心特性

- **高并发 TCP 长连接**：基于 `libevent` 事件驱动框架，单机可支撑数万并发连接，支持心跳保活与自动超时断开。
- **标准化 gRPC 接口**：提供 Protobuf 协议定义的 RPC 服务，方便各类业务后端无缝接入。
- **消息队列解耦**：推送请求首先写入 Apache Kafka，由消费者异步处理，实现削峰填谷与系统解耦。
- **智能路由与状态管理**：利用 Redis 存储用户在线状态及所在推送服务器地址，支持多实例水平扩展与跨节点消息转发。
- **离线消息持久化**：用户离线时消息自动存入 MySQL，为后续历史消息拉取与补发提供数据支撑。
- **线程安全设计**：关键组件均采用互斥锁保护，支持多线程并发调用。

---

### 数据流说明

1. **消息入口**：业务后端通过 gRPC 调用 `PushService.PushMessage` 接口，传入目标用户 ID 和消息内容。
2. **消息缓冲**：Push Server 将消息立即投递至 Kafka 主题 `push_msg_topic`，并返回受理成功响应，实现接口快速返回。
3. **异步消费**：内置的 Kafka 消费者线程从 Kafka 拉取消息，根据用户在线状态执行不同分支：
   - **在线**：通过 TCP 长连接实时推送给客户端。
   - **离线**：将消息写入 MySQL `offline_msg` 表，待用户上线后由业务方拉取补发。
4. **状态同步**：客户端 TCP 连接建立、心跳维持、断开时，Push Server 实时更新 Redis 中的用户在线状态及所在服务器地址，供多实例路由查询。

---

## 项目目录结构

```
.
├── common.h                 # 公共头文件（常量、数据结构、依赖库头文件）
├── push_server.h / .cpp     # TCP 推送服务器（基于 libevent）
├── push_grpc.h / .cpp       # gRPC 服务端实现
├── redis_client.h / .cpp    # Redis 客户端封装（hiredis，线程安全）
├── kafka_client.h / .cpp    # Kafka 生产者封装（librdkafka++）
├── kafka_consumer.h / .cpp  # Kafka 消费者封装（异步处理消息）
├── mysql_client.h / .cpp    # MySQL 客户端封装（线程安全）
├── main.cpp                 # 主程序入口
├── push.proto               # Protobuf 协议定义文件（需自行补充或还原）
├── push.pb.h / .cc          # Protobuf 生成的消息类
├── push.grpc.pb.h / .cc     # gRPC 生成的服务类
└── push_db.sql              # MySQL 数据库建表脚本
```

> 注：项目中已包含 `push.pb.*` 和 `push.grpc.pb.*` 生成文件，可直接使用。若需修改协议，请参考下文“编译与运行”章节重新生成。

---

## 依赖环境

### 必需库

| 名称               | 用途                   | 
|--------------------|------------------------|
| libevent           | 异步网络事件处理       | 
| gRPC & Protobuf    | RPC 通信与序列化       | 
| hiredis            | Redis C 客户端         | 
| librdkafka         | Kafka C/C++ 客户端     | 
| MySQL Connector/C  | MySQL 数据库连接       | 

### 服务依赖

- **Redis**：建议 5.0+
- **Kafka**：建议 2.8+
- **MySQL**：建议 5.7+

---

## 编译与运行

### 1. 准备 Protobuf 协议文件

若仓库中缺失 `push.proto`，可根据现有 `.pb.h` 内容还原，创建 `push.proto`：

```protobuf
syntax = "proto3";

package push;

message PushRequest {
    string user_id = 1;
    string content = 2;
}

message PushResponse {
    int32 code = 1;
    string msg = 2;
}

service PushService {
    rpc PushMessage(PushRequest) returns (PushResponse);
}
```

如需重新生成 C++ 代码，执行：

```bash
protoc --cpp_out=. push.proto
protoc --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` push.proto
```

### 2. 配置依赖服务地址

修改 `common.h` 中的连接常量以匹配您的环境：

```cpp
const int SERVER_PORT = 8888;                     // TCP 监听端口
const int GRPC_PORT = 50051;                      // gRPC 端口
const int HEARTBEAT_TIMEOUT = 30;                 // 心跳超时秒数

const std::string REDIS_HOST = "127.0.0.1";
const int REDIS_PORT = 6379;

const std::string KAFKA_BROKER = "127.0.0.1:9092";
const std::string KAFKA_TOPIC = "push_msg_topic";
const std::string KAFKA_GROUP_ID = "push_consumer_group";

const std::string MYSQL_HOST = "127.0.0.1";
const std::string MYSQL_USER = "root";
const std::string MYSQL_PASS = "123456";
const std::string MYSQL_DB = "push_db";
```

### 3. 初始化 MySQL 数据库

执行提供的 SQL 脚本创建数据库和离线消息表：

```bash
mysql -u root -p < push_db.sql
```

脚本内容：

```sql
CREATE DATABASE IF NOT EXISTS push_db;
USE push_db;

CREATE TABLE IF NOT EXISTS offline_msg (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id VARCHAR(64) NOT NULL,
    content TEXT,
    create_time DATETIME DEFAULT NOW()
);
```


### 4. 启动服务

确保 Redis、Kafka、MySQL 均已启动后运行，成功启动后会输出类似日志：

```
redis connect success
kafka producer init success
mysql connect success
Kafka consumer initialized, subscribed to topic: push_msg_topic
Push server started on 0.0.0.0:8888
gRPC server started on 0.0.0.0:50051
```

---

## 使用说明

### 客户端 TCP 协议

客户端需通过 TCP 连接 Push Server 的 `8888` 端口，并遵循以下简单文本协议：

#### 登录与心跳

发送以下格式的字符串（以换行或 `\0` 结尾均可）：

```
uid=<your_user_id>
```

服务端收到后会将当前连接与用户 ID 绑定，并更新 Redis 在线状态。  
心跳维持：客户端可定时发送任意数据（如 `PING`）或重复发送 `uid=...`，每次收到数据都会刷新心跳时间（默认超时 30 秒）。

#### 接收推送

当有消息推送给该用户时，服务端会直接通过 TCP 连接发送消息原文（不附加任何协议头）。

---

## 配置说明

所有可调整的运行时参数均集中在 `common.h` 中：

| 常量名                 | 默认值                 | 说明                                   |
|------------------------|------------------------|----------------------------------------|
| `SERVER_PORT`          | 8888                   | TCP 长连接监听端口                     |
| `GRPC_PORT`             | 50051                  | gRPC 服务端口                          |
| `HEARTBEAT_TIMEOUT`     | 30                     | 心跳超时时间（秒），超时后自动断开连接 |
| `REDIS_HOST` / `PORT`   | 127.0.0.1 / 6379       | Redis 服务器地址                       |
| `KAFKA_BROKER`          | 127.0.0.1:9092         | Kafka Broker 地址                      |
| `KAFKA_TOPIC`           | push_msg_topic         | Kafka 主题名称                         |
| `KAFKA_GROUP_ID`        | push_consumer_group    | Kafka 消费者组 ID                      |


---

## 线程安全说明

项目在多线程环境下经过精心设计，确保数据安全：

- **PushServer**：使用 `std::mutex` 保护 `conn_map_` 和 `user_map_`，所有回调函数内部均加锁操作。
- **RedisClient**：所有 Redis 命令执行均使用互斥锁保护 `redisContext` 指针。
- **MySQLClient**：`InsertOfflineMsg` 方法内部加锁，防止并发写入导致异常。
- **KafkaConsumer**：独立运行在单独线程中，通过 `PushServer` 和 `MySQLClient` 的公开线程安全接口交互。

---

## 性能与扩展性

### 单机性能

- TCP 并发连接数：受系统文件描述符限制，调整 `ulimit -n` 后可轻松支持数万连接。
- 消息吞吐量：依赖于 Kafka 生产者和消费者的批处理配置，典型场景下可达数万 QPS。

### 水平扩展

- **无状态设计**：Push Server 本身不存储持久化数据，所有状态均保存在 Redis 和 Kafka 中。
- **路由发现**：通过 Redis 的 `online_map`（Hash）可查询任意用户当前连接的服务器地址，支持跨节点消息转发（转发逻辑需业务方自行实现或扩展 gRPC 接口）。
- **Kafka 分区**：建议将 `push_msg_topic` 的分区数设置为与消费者实例数匹配，以实现并行消费。

