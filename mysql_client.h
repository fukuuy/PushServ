#pragma once
#include <string>
#include <mysql.h>

class MySQLClient {
public:
    static MySQLClient& Instance();
    bool Connect();
    void InsertOfflineMsg(const std::string& uid, const std::string& content);

private:
    MySQLClient() = default;
    MYSQL* mysql_ = nullptr;
};