CREATE DATABASE push_db;
USE push_db;

CREATE TABLE offline_msg (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id VARCHAR(64) NOT NULL,
    content TEXT,
    create_time DATETIME DEFAULT NOW()
);