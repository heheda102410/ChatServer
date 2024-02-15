#include "friendmodel.hpp"
// 添加好友关系
void FriendModel::insert(ConnPool* pool,int userid, int friendid) {
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values (%d, %d)", userid, friendid);
    // 2.执行sql语句
    shared_ptr<MysqlConn> conn = pool->getConn();
    conn->update(sql);
}

//返回用户好友列表:返回用户好友id、名称、登录状态信息
vector<User> FriendModel::query(ConnPool* pool,int userid) {
    // 1.组装sql语句
    char sql[1024] = {0};
    // sprintf(sql, "select a.id, a.name, a.state from user a inner join friend b on b.friendid = a.id where b.userid = %d", userid);      
    sprintf(sql, "select a.id, a.name, a.state from user a inner join friend b on b.userid = a.id where b.friendid = %d \
            union (select a.id, a.name, a.state from user a inner join friend b on b.friendid = a.id where b.userid = %d \
            or b.friendid = %d and a.id!=%d)",userid,userid,userid,userid);     
    // 2.发送SQL语句,进行相应处理
    vector<User> vec;
    shared_ptr<MysqlConn> conn = pool->getConn();
    MYSQL_RES * res = conn->query(sql);
    if(res != nullptr) {
        // 把userid用户的所有离线消息放入vec中返回
        MYSQL_ROW row;
        //将userid好友的详细信息返回
        while((row = mysql_fetch_row(res)) != nullptr) {
            User user;
            user.setId(atoi(row[0])); // id
            user.setName(row[1]);     // name
            user.setState(row[2]);    // state
            vec.push_back(user);
        }
        mysql_free_result(res);       // 释放资源
        return vec;
    }
    return vec;
}

// select a.id,a.name,a.state from user a inner join 
// friend b on b.friendid = a.id 
// where b.userid = %d