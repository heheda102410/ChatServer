#include "db.h"
#include <muduo/base/Logging.h>
// 数据库配置信息
static string server = "127.0.0.1";
static string user = "root";
static string password = "123456";
static string dbname = "chat";

// 初始化数据库连接:开辟存储连接的资源空间
Mysql::Mysql() {
    m_conn = mysql_init(nullptr);
    // 这里相当于只是给它开辟了一块存储连接数据的资源空间
}

// 释放数据库连接资源:释放存储连接的资源空间
Mysql::~Mysql() {
    if(m_conn != nullptr) {
        mysql_close(m_conn);
    }
    // 析构的时候把这块资源空间用mysql_close掉
}

// 连接数据库
bool Mysql::connect() {
    MYSQL *p = mysql_real_connect(m_conn,server.c_str(),user.c_str(),
                                  password.c_str(),dbname.c_str(),3306,nullptr,0);
    if(p!=nullptr) {
        // C和C++代码默认的编码字符是ASCII,如果不设置,
        // 从MYSQL上拉下来的中文显示?
        mysql_query(m_conn, "set names gbk");
        LOG_INFO << "connect mysql success!!!";
    } else{
        LOG_INFO << "connect mysql failed!!!";
    }
    return p;
}

// 更新操作
bool Mysql::update(string sql) {
    if(mysql_query(m_conn, sql.c_str())) {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":" 
            << sql <<"更新失败!";
        return false;
    }
    return true;
}

// 查询操作
MYSQL_RES* Mysql::query(string sql) {
    if(mysql_query(m_conn, sql.c_str())) {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
            << sql <<"查询失败!";   
        return nullptr;
    }
    return mysql_use_result(m_conn);
}

// 获取连接
MYSQL* Mysql::getConnection() {
    return m_conn;
}