#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>//条件变量
#include "MysqlConn.h"
using namespace std;
class ConnPool {
public:
    static ConnPool* getConnPool();// 获得单例对象
    ConnPool(const ConnPool& obj) = delete; // 删除拷贝构造函数
    ConnPool& operator=(const ConnPool& obj) = delete; // 删除拷贝赋值运算符重载函数
    shared_ptr<MysqlConn> getConn(); // 从连接池中取出一个连接
    ~ConnPool(); // 析构函数
private:
    ConnPool(); // 构造函数私有化
    bool parseJsonFile(); // 解析json格式文件
    void produceConn(); // 生产数据库连接
    void recycleConn(); // 销毁数据库连接
    void addConn(); // 添加数据库连接
 
    // 连接服务器所需信息
    string m_ip;            // 数据库服务器ip地址
    string m_user;          // 数据库服务器用户名
    string m_dbName;        // 数据库服务器的数据库名
    string m_passwd;        // 数据库服务器密码
    unsigned short m_port;  // 数据库服务器绑定的端口
 
    // 连接池信息
    queue<MysqlConn*> m_connQ;
    unsigned int m_maxSize; // 连接数上限值
    unsigned int m_minSize; // 连接数下限值
    int m_timeout; // 连接超时时长
    int m_maxIdleTime; // 最大的空闲时长
    mutex m_mutexQ; // 独占互斥锁
    condition_variable m_cond; // 条件变量
};