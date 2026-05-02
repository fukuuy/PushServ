#include "common.h"
#include "mysql_client.h"
#include <iostream>
#include <vector>

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
    std::lock_guard<std::mutex> lock(mutex_);
    if (!mysql_) return;

    // 先对 uid 和 content 做转义，防止 SQL 注入
    std::vector<char> uid_esc(uid.size() * 2 + 1);
    std::vector<char> content_esc(content.size() * 2 + 1);
    mysql_real_escape_string(mysql_, uid_esc.data(), uid.c_str(), static_cast<unsigned long>(uid.size()));
    mysql_real_escape_string(mysql_, content_esc.data(), content.c_str(), static_cast<unsigned long>(content.size()));

    char sql[4096];
    snprintf(sql, sizeof(sql),
        "INSERT INTO offline_msg(user_id,content) VALUES ('%s','%s')",
        uid_esc.data(), content_esc.data());
    mysql_query(mysql_, sql);
}

