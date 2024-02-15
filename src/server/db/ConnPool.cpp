#include "ConnPool.h"
// #include <json/json.h>
// #include <json.h>
#include "json.hpp"
#include <fstream>
#include <thread>
#include <iostream>
// using namespace Json;
using json = nlohmann::json;
ConnPool* ConnPool::getConnPool() {
    static ConnPool pool;
    return &pool;
}
 
// 从连接池中取出一个连接
shared_ptr<MysqlConn> ConnPool::getConn() {
    unique_lock<mutex> locker(m_mutexQ);
    while (m_connQ.empty()) {
        if (cv_status::timeout == m_cond.wait_for(locker, chrono::milliseconds(m_timeout))) {
            if (m_connQ.empty()) {
                //return nullptr;
                continue;
            }
        }
    }
    shared_ptr<MysqlConn>connptr(m_connQ.front(), [this](MysqlConn* conn) {
        lock_guard<mutex>locker(m_mutexQ); // 自动管理加锁和解锁
        conn->refreshAliveTime();// 更新连接的起始的空闲时间点
        m_connQ.push(conn); // 回收数据库连接，此时它再次处于空闲状态
        });// 智能指针
    m_connQ.pop();
    m_cond.notify_one(); // 本意是唤醒生产者
    return connptr;
}
 
ConnPool::~ConnPool() {
    while (!m_connQ.empty()) {
        MysqlConn* conn = m_connQ.front();
        m_connQ.pop();
        delete conn;
    }
}
 
ConnPool::ConnPool() {
    // 加载配置文件
    if (!parseJsonFile()) {
        std::cout << "加载配置文件失败！！！" << std::endl;
        return;
    }
    for (int i = 0; i < m_minSize; ++i) {
        addConn();
    }
    thread producer(&ConnPool::produceConn, this);// 生产连接
    thread recycler(&ConnPool::recycleConn, this);// 销毁连接
    producer.detach();
    recycler.detach();
}
 
bool ConnPool::parseJsonFile() {
    ifstream ifs;
    ifs.open("/home/heheda/Linux/Chat/configuration/dbconf.json");
    if (!ifs.is_open()) {
        std::cout << "无法打开 dbconf.json 配置文件！";
        return false;
    }
    std::cout << "开始解析 dbconf.json 配置文件..." << std::endl;
    json data; // 创建一个空的JSON对象
    ifs>>data; // 将文件内容加载到JSON对象中
    m_ip = data["ip"];
    m_port = data["port"];
    m_user = data["userName"];
    m_passwd = data["password"];
    m_dbName = data["dbName"];
    m_minSize = data["minSize"];
    m_maxSize = data["maxSize"];
    m_maxIdleTime = data["maxIdleTime"];
    m_timeout = data["timeout"];
    /*
    ifstream ifs("dbconf.json");
    Reader rd;
    Value root;
    rd.parse(ifs, root);
    if (root.isObject()) {
        std::cout << "开始解析配置文件..." << std::endl;
        m_ip = root["ip"].asString();
        m_port = root["port"].asInt();
        m_user = root["userName"].asString();
        m_passwd = root["password"].asString();
        m_dbName = root["dbName"].asString();
        m_minSize = root["minSize"].asInt();
        m_maxSize = root["maxSize"].asInt();
        m_maxIdleTime = root["maxIdleTime"].asInt();
        m_timeout = root["timeout"].asInt();
        return true;  // 解析成功返回true，否则返回false。
    }
    return false;
    */
    return true;
}
 
void ConnPool::produceConn() {
    while (true) {  // 生产者线程不断生产连接，直到连接池达到最大值
        unique_lock<mutex> locker(m_mutexQ);  // 加锁，保证线程安全
        while (m_connQ.size() >= m_minSize) {
            m_cond.wait(locker);  // 等待消费者通知
        }
        addConn(); // 生产连接
        m_cond.notify_all();// 通知消费者(唤醒)
    }
}
 
// 回收数据库连接
void ConnPool::recycleConn() {
    while (true) {
        this_thread::sleep_for(chrono::milliseconds(500));// 每隔半秒钟检测一次
        lock_guard<mutex> locker(m_mutexQ);  // 加锁，保证线程安全
        while (m_connQ.size() > m_minSize) {  // 如果连接池中的连接数大于最小连接数，则回收连接
            MysqlConn* conn = m_connQ.front();  // 取出连接池中的连接
            if (conn->getAliveTime() >= m_maxIdleTime) {
                m_connQ.pop();  // 回收连接
                delete conn;  // 释放连接资源
            }
            else {
                break;  // 如果连接的空闲时间小于最大空闲时间，则跳出循环
            }
        }
    }
}
 
// 添加连接到连接池
void ConnPool::addConn() {
    MysqlConn* conn = new MysqlConn;
    conn->connect(m_user, m_passwd, m_dbName, m_ip, m_port);
    conn->refreshAliveTime();// 记录建立连接的时候的对应的时间戳
    m_connQ.push(conn);
}