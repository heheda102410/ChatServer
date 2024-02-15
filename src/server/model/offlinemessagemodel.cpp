#include "offlinemessagemodel.hpp"
// 存储用户的离线消息
void OfflineMsgModel::insert(ConnPool* pool,int userid, string msg) {
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage values(%d, '%s')", userid, msg.c_str());
    // 2.执行sql语句
    shared_ptr<MysqlConn> conn = pool->getConn();
    conn->update(sql);
}

// 删除用户的离线消息
void OfflineMsgModel::remove(ConnPool* pool,int userid) {
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid = %d", userid);
    // 2.执行sql语句
    shared_ptr<MysqlConn> conn = pool->getConn();
    conn->update(sql);
}

// 查询用户的离线消息:离线消息可能有多个
vector<string> OfflineMsgModel::query(ConnPool* pool,int userid) {
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid = %d", userid);
    // 2.执行sql语句
    vector<string> vec;// 存储离线消息,离线消息可能有多条
    shared_ptr<MysqlConn> conn = pool->getConn();
    MYSQL_RES *res = conn->query(sql);
    if(res != nullptr) {
        // 把userid用户的所有离线消息放入vec中返回
        MYSQL_ROW row;
        while((row = mysql_fetch_row(res)) != nullptr) { //循环查找离线消息
            vec.push_back(row[0]);
        }
        mysql_free_result(res);
        return vec;
    }
    return vec;
}
