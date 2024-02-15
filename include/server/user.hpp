#ifndef USER_H
#define USER_H

#include <string>
using namespace std;

// 匹配User表的ORM类
class User {
public:
    User(int id=-1, string name="", string password="", string state="offline") {
        m_id = id;
        m_name = name;
        m_password = password;
        m_state = state;
    }
    // 设置相应字段
    void setId(int id) { m_id = id; }
    void setName(string name) { m_name = name; }
    void setPwd(string pwd) { m_password = pwd; }   
    void setState(string state) { m_state = state; }
    
    // 获取相应字段
    int getId() const { return m_id; }
    string getName() const { return m_name; }
    string getPwd() const { return m_password; }
    string getState() const { return m_state; }
private:
    int m_id;            // 用户id
    string m_name;       // 用户名
    string m_password;   // 用户密码
    string m_state;      // 当前登录状态
};
#endif // USER_H

/*
数据层代码框架设计
数据库操作与业务代码进行分离,业务代码处理的都为对象,数据库层操作
具体SQL语句,因此我们定义相应的类，每一个类对应数据库中一张表，将
数据库读出来的字段提交给业务使用。
*/