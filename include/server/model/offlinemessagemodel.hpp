#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H
#include <string>
#include <vector>
#include "ConnPool.h"
using namespace std;

// 离线消息表的数据操作类:针对表的增删改查(提供离线消息表的操作接口方法)
class OfflineMsgModel {
public:
    // 存储用户的离线消息
    void insert(ConnPool* pool,int userid, string msg);
    // 删除用户的离线消息
    void remove(ConnPool* pool,int userid);
    // 查询用户的离线消息:离线消息可能有多个
    vector<string> query(ConnPool* pool,int userid);
};

#endif // OFFLINEMESSAGEMODEL_H