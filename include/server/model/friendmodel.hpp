#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include "user.hpp"
#include "ConnPool.h"
#include <vector>
using namespace std;

// Friend用户表的数据操作类:针对类的增删改查(维护好友信息的操作接口方法)
class FriendModel {
public:
    // 添加好友关系
    void insert(ConnPool* pool,int userid, int friendid);
    // 返回用户好友列表:返回用户好友id,名称,登录状态信息 
    vector<User> query(ConnPool* pool,int userid);
};

#endif // FRIENDMODEL_H