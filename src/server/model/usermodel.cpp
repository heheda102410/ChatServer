#include "usermodel.hpp"
#include "MysqlConn.h"
#include <iostream>
#include <memory>
// User表的增加方法
bool UserModel::insert(ConnPool* pool,User &user) {
    // 1.组装sql语句
    char sql[1024] = {0};
    std::sprintf(sql,"insert into user(name,password,state) values('%s','%s', '%s')",
         user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());
    // 2.执行sql语句,进行处理
    shared_ptr<MysqlConn> conn = pool->getConn();
    if(conn->update(sql)) {
        // 获取插入成功的用户数据生成的主键id
        // id为自增键,设置回去user对象添加新生成的用户id
        user.setId(mysql_insert_id(conn->getConnection()));
        return true;
    }
    return false;
}

// 根据用户号码查询用户信息
User UserModel::query(ConnPool* pool,int id) {
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql,"select * from user where id = %d", id);
    // 2.执行sql语句
    shared_ptr<MysqlConn> conn = pool->getConn();
    // 查询id对应的数据
    MYSQL_RES* res = conn->query(sql);
    if(res != nullptr) { // 查询成功
        MYSQL_ROW row = mysql_fetch_row(res);// 获取行数据
        if(row != nullptr) {
            User user;
            user.setId(atoi(row[0]));
            user.setName(row[1]);
            user.setPwd(row[2]);
            user.setState(row[3]);
            // 释放res动态开辟的资源
            mysql_free_result(res);
            return user;// 返回user对应的信息
        }
    }
    return User(); // 未找到,返回默认的user对象
}

// 更新用户的状态信息
bool UserModel::updateState(ConnPool* pool,User user) {
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql,"update user set state = '%s' where id = %d",
         user.getState().c_str(), user.getId());
    // 2.执行sql语句
    shared_ptr<MysqlConn> conn = pool->getConn();
    if(conn->update(sql)) {
        return true;
    }
    return false;
}

// 重置用户的状态信息
void UserModel::resetState(ConnPool* pool) {
    // 1.组装sql语句
    char sql[1024] = "update user set state = 'offline' where state = 'online'";
    // 2.执行sql语句,进行相应处理
    shared_ptr<MysqlConn> conn = pool->getConn();
    conn->update(sql);
}
