#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.hpp"
#include <string>
#include <vector>
using namespace std;
#include "ConnPool.h"
// 群组表的数据操作类:维护数组信息的操作接口方法
class GroupModel {
public:
    // 创建数组
    bool createGroup(ConnPool* pool,Group &group);
    // 加入群组
    void joinGroup(ConnPool* pool,int userid, int groupid, string role);
    // 查询用户所在群组信息
    vector<Group> queryGroups(ConnPool* pool,int userid);
    // 根据指定的groupid查询群组用户id列表,除userid自己,主要用户群聊业务给群组其他成员群发消息
    vector<int> queryGroupUsers(ConnPool* pool,int userid, int groupid);
};

#endif // GROUPMODEL_H