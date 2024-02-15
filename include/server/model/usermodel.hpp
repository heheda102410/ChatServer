#ifndef USERMODEL_H
#define USERMODEL_H
#include "user.hpp"
#include "ConnPool.h"
// User表的数据操作类:针对表的增删改查
class UserModel {
public:
    // user表的增加方法
    bool insert(ConnPool* pool,User& user); 
    // 根据用户号码查询用户信息
    User query(ConnPool* pool,int id);
    // 更新用户的状态信息
    bool updateState(ConnPool* pool,User user);
    // 重置用户的状态信息
    void resetState(ConnPool* pool);
};

#endif // USERMODEL_H