#include "MysqlConn.h"
#include <muduo/base/Logging.h> 
// 初始化数据库连接
MysqlConn::MysqlConn() {
    m_conn = mysql_init(nullptr);
    mysql_set_character_set(m_conn, "GBK"); // 设置字符集
}
 
// 释放数据库连接
MysqlConn::~MysqlConn() {
    if (m_conn != nullptr) {
        mysql_close(m_conn);
    }
    freeResult();
}
 
// 连接数据库
bool MysqlConn::connect(string user, string passwd, string dbName, string ip, unsigned short port) {
    MYSQL* ptr = mysql_real_connect(m_conn, ip.c_str(), user.c_str(), passwd.c_str(), dbName.c_str(), port, nullptr, 0);
    return ptr != nullptr;
}
 
// 更新数据库:insert，update，delete
bool MysqlConn::update(string sql) {
    if (mysql_query(m_conn, sql.c_str())) {
        return false;
    }
    return true;
}
 
// 查询数据库
MYSQL_RES* MysqlConn::query(string sql) {
    if(mysql_query(m_conn, sql.c_str())) {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
            << sql <<"查询失败!";   
        return nullptr;
    }
    return mysql_use_result(m_conn);
}
 
// 遍历查询得到的结果集
bool MysqlConn::next() {
    if (m_result != nullptr) {
        m_row = mysql_fetch_row(m_result);
        if (m_row != nullptr) {
            return true;
        }
    }
    return false;
}
 
// 得到结果集中的字段值
string MysqlConn::value(int index) {
    int rowCount = mysql_num_fields(m_result);
    if (index >= rowCount || index < 0) {
        return string();
    }
    char* val = m_row[index];
    unsigned long length = mysql_fetch_lengths(m_result)[index];
    return string(val, length);
}
 
// 事务操作
bool MysqlConn::transaction() {
    return mysql_autocommit(m_conn, false);
}
 
// 提交事务
bool MysqlConn::commit() {
    return mysql_commit(m_conn);
}
 
// 事务回滚
bool MysqlConn::rollback() {
    return mysql_rollback(m_conn);
}
 
// 刷新起始的空闲时间点
void MysqlConn::refreshAliveTime() {
    // 这个时间戳就是某个数据库连接，它起始存活的时间点
    // 这个时间点通过时间类就可以得到了
    m_aliveTime = steady_clock::now();
}
 
// 计算连接存活的总时长
long long MysqlConn::getAliveTime() {
    nanoseconds duration = steady_clock::now() - m_aliveTime;
    milliseconds millsec = duration_cast<milliseconds>(duration);
    return millsec.count();
}

// 获取连接
MYSQL *MysqlConn::getConnection() {
    return m_conn;
}

void MysqlConn::freeResult() {
    if (m_result != nullptr) {
        mysql_free_result(m_result);
        m_result = nullptr;
    }
}