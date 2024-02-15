#ifndef GROUPUSER_H
#define GROUPUSER_H
#include <string>
#include "user.hpp"
using namespace std;

// 群组用户,多了一个role角色信息,从User类直接继承,复用User的其他信息
// GroupUser群组员表的映射类:映射表的相应字段
class GroupUser : public User {
public:
    void setRole(string role) { m_role = role; }
    string getRole() { return m_role; }
private:
    string m_role;
};

#endif // GROUPUSER_H