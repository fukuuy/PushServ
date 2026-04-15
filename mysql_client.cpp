#include "common.h"
#include "mysql_client.h"
#include <iostream>

MySQLClient& MySQLClient::Instance() {
    static MySQLClient ins;
    return ins;
}

bool MySQLClient::Connect() {
    mysql_ = mysql_init(nullptr);
    if (!mysql_) return false;
    return mysql_real_connect(mysql_,
        MYSQL_HOST.c_str(),
        MYSQL_USER.c_str(),
        MYSQL_PASS.c_str(),
        MYSQL_DB.c_str(),
        3306, nullptr, 0) != nullptr;
}

void MySQLClient::InsertOfflineMsg(const std::string& uid, const std::string& content) {
    if (!mysql_) return;
    char sql[2048];
    snprintf(sql, sizeof(sql),
        "INSERT INTO offline_msg(user_id,content) VALUES ('%s','%s')",
        uid.c_str(), content.c_str());
    mysql_query(mysql_, sql);
}

