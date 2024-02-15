#ifndef DB_H
#define DB_H

#include <mysql/mysql.h>
#include <string>
using namespace std;

// 数据库操作类
class Mysql {
public:
    // 初始化数据库连接:开辟存储连接的资源空间
    Mysql();
    // 释放数据库连接资源:释放存储连接的资源空间
    ~Mysql();
    // 连接数据库
    bool connect();
    // 更新操作
    bool update(string sql);
    // 查询操作
    MYSQL_RES *query(string sql);
    // 获取连接
    MYSQL *getConnection();
private:
    MYSQL *m_conn;// 与MySQL Server的一条连接
};

#endif // DB_H

/*
服务器数据模块:将数据库数据与业务模块代码区分开来,符合ORM(对象关系映射)
框架设计,业务层操作的都是数据层的对象,数据层封装数据库SQL相应的操作
*/

