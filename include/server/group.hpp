#ifndef GROUP_H
#define GROUP_H
#include <vector>
#include <string>
using namespace std;
#include "groupuser.hpp"
// User表的ORM类
// Group群组表的映射类:映射表的相应字段
class Group{
public:
    Group(int id=-1,string name="",string desc="") 
        : m_id(id)
        ,m_name(name)
        ,m_desc(desc) {
        
    }

    void setId(int id) { m_id = id; }
    void setName(string name) { m_name = name; }
    void setDesc(string desc) { m_desc = desc; }
    
    int getId() const { return m_id; }
    string getName() const { return m_name; }
    string getDesc() const { return m_desc; }
    vector<GroupUser> &getUsers()  { return m_users; }

private:
    int m_id;                 // 群组id
    string m_name;            // 群组名称
    string m_desc;            // 群组功能描述
    vector<GroupUser> m_users;// 存储组成员
};

#endif // GROUP_H