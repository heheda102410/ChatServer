#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <vector>
#include <map>
#include <string>
#include <string.h>
#include <iostream>
using namespace std;
using namespace muduo;

// 获取单例对象的接口函数 线程安全的单例对象
ChatService* ChatService::getInstance() {
    static ChatService service;
    return &service;
}

// 构造函数:注册消息以及对应的Handler回调操作 实现网络模块与业务模块解耦的核心
// 将群组业务的消息id分别与对应的事件处理器提前在聊天服务器业务类的构造函数里绑定好
ChatService::ChatService() {
    m_msgHandlerMap.insert({LOGIN_MSG,std::bind(&ChatService::login, this, _1, _2, _3)});  
    m_msgHandlerMap.insert({REG_MSG,std::bind(&ChatService::reg, this, _1, _2, _3)});  
    m_msgHandlerMap.insert({ONE_CHAT_MSG,std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    // m_msgHandlerMap.insert({ADD_FRIEND_MSG,std::bind(&ChatService::addFriend, this, _1, _2, _3)}); 
    m_msgHandlerMap.insert({ADD_FRIEND_REQ_MSG,std::bind(&ChatService::addFriendRequest, this, _1, _2, _3)});  
    m_msgHandlerMap.insert({ADD_FRIEND_MSG_ACK,std::bind(&ChatService::addFriendResponse, this, _1, _2, _3)});

    m_msgHandlerMap.insert({LOGIN_OUT_MSG, std::bind(&ChatService::loginOut, this, _1, _2, _3)});
    m_msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    m_msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::joinGroup, this, _1, _2, _3)});
    m_msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
    // 连接redis服务器
    if(m_redis.connect()) {
        // 设置上报消息的回调 
        m_redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));  
    }
    // 初始化数据库
    m_connPool = ConnPool::getConnPool();
}

// 处理登录业务  user表:id password字段
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    // 1.获取ids,password字段
    int id = js["id"].get<int>();
    string pwd = js["password"];

    // 传入用户id,返回相应数据
    ConnPool* connPool = this->getConnPool();
    User user = m_userModel.query(connPool,id);
    if(user.getId() == id && user.getPwd() == pwd) { // 登录成功
        if(user.getState() == "online") {
            //该用户已经登录,不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2; // 重复登录
            // response["errmsg"] = "该账号已经登录,请重新输入新账号";
            response["errmsg"] = "this account has logined, please input a new account";    
            conn->send(response.dump());
        }
        else{ // 用户未登录,此时登录成功
            // 登录成功,记录用户连接信息
            /*
            在用户登录成功时便将用户id与连接信息记录在一个map映射表里,方便后续查找与使用
            线程安全问题:上述我们虽然建立了用户id与连接的映射,但是在多线程环境下,不同的用户
            可能会在不同的工作线程中调用同一个业务,可能同时有多个用户上线,下线操作,因此要
            保证map表的线程安全
            */
            {
                lock_guard<mutex> lock(m_connMutex);
                m_userConnMap.insert({id, conn}); // 登录成功记录用户连接信息
            }
            // id用户登录成功后,向redis订阅channel(id)通道的事件
            m_redis.subscribe(id);

            // 登录成功,更新用户状态信息 state: offline => online
            user.setState("online");
            m_userModel.updateState(connPool,user); // 更新用户状态信息

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();
            
            // 查询该用户是否有离线消息
            vector<string> vec = m_offlineMsgModel.query(connPool,id);
            if(!vec.empty()) {
                response["offlinemsg"] = vec;// 查询到离线消息，发送给用户
                cout<<"查询到离线消息，发送给用户 :" <<response["offlinemsg"]<<endl;
                // 读取该用户的离线消息后,把该用户的所有离线消息删除掉
                m_offlineMsgModel.remove(connPool,id);
            }
            // 登录成功,查询该用户的好友信息并返回
            vector<User>userVec = m_friendModel.query(connPool,id);
            if(!userVec.empty()) {
                vector<string> vec2;
                for(User &user : userVec) {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            vector<Group> groupVec = m_groupModel.queryGroups(connPool,id);
            if(groupVec.size() > 0) {
                // cout<<"................sdsdfasas................."<<endl;
                vector<string> vec3;
                for(Group& group:groupVec) {
                    vector<GroupUser> users = group.getUsers();
                    json js;
                    js["id"] = group.getId();
                    js["groupname"] = group.getName();
                    js["groupdesc"] = group.getDesc();

                    vector<string> userVec;
                    for(GroupUser& user:users) {
                        json js_tmp;
                        js_tmp["id"] = user.getId();
                        js_tmp["name"] = user.getName();
                        js_tmp["state"] = user.getState();
                        js_tmp["role"] = user.getRole();
                        userVec.push_back(js_tmp.dump());
                    }
                    js["users"] = userVec;
                    vec3.push_back(js.dump());
                    // cout<<"js.dump() = "<<js.dump()<<endl;
                }
                response["groups"] = vec3;
            }
            conn->send(response.dump());
        }
    }
    else {
        // 该用户不存在/用户存在但是密码错误,登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        // response["errmsg"] = "该用户不存在,您输入用户名或者密码可能错误!";
        response["errmsg"] = "This user does not exist, or the password you entered may be incorrect!"; 
        conn->send(response.dump());
    }
}

// 处理注册业务 user表:name password
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    // 1.获取name,password字段
    string name = js["name"];
    string pwd = js["password"];

    // 处理业务,操作的都是数据对象
    // 2.创建User对象,进行注册
    User user;
    user.setName(name);
    user.setPwd(pwd);
    // 新用户的插入
    ConnPool* connPool = this->getConnPool();
    bool state = m_userModel.insert(connPool,user);
    if(state) { // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK; // 注册响应消息
        response["errno"] = 0;           // 错误标识 0:成功 1:失败
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else { // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

// 处理一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    // 1.先获取目的id
    int toid = js["toid"].get<int>();
    {
        lock_guard<mutex> lock(m_connMutex);
        auto it = m_userConnMap.find(toid);
        // 2.目的id在线 进行消息转发,服务器将源id发送的消息中转给目的id
        if(it != m_userConnMap.end()) {
            // toid在线,转发消息  服务器主动推送消息给toid用户
            it->second->send(js.dump());
            return;
        }
    }
    
    // 查询toid是否在线
    /*
     * A向B说话,在map表中未找到B,B可能不在本台服务器上但通过
     * 数据库查找在线,要发送的消息直接发送以B用户为id的通道上;
     * 也可能是离线状态,发送离线消息
     */

    cout<<"发送消息 :" <<js.dump()<<endl;

    ConnPool* connPool = this->getConnPool();
    User user = m_userModel.query(connPool,toid);
    if(user.getState() == "online") {
        m_redis.publish(toid, js.dump());
        return;
    }

    // 目的id不在线，将消息存储到离线消息里
    m_offlineMsgModel.insert(connPool,toid, js.dump());
}

// 添加好友业务 msgid id friendid
// void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time) {
//     std::cout<<"添加好友业务 msgid id friendid"<<std::endl;
//     // 1.获取当前用户id,要添加好友id
//     int userid = js["id"].get<int>();
//     int friendid = js["friendid"].get<int>();
//     std::cout<<"打印当前用户id:"<<userid<<std::endl;
//     std::cout<<"打印要添加好友id:"<<friendid<<std::endl;
//     // 2.数据库中存储要添加好友的信息
//     ConnPool* connPool = this->getConnPool();
//     m_friendModel.insert(connPool,userid, friendid);
// }
// 添加好友业务请求
void ChatService::addFriendRequest(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();
    json response;
    response["msgid"] = ADD_FRIEND_REQ_MSG;

    string msgStr = "用户ID: "+to_string(userid)+" ,请求添加您为好友"+to_string(friendid);
    response["msg"] = msgStr;
    response["from"] = userid;
    response["toid"] = friendid;
    std::cout<<"来到这里了:"<<response.dump()<<std::endl;
    oneChat(conn,response,time);
}
 
// 添加好友业务 msgid id friendid
void ChatService::addFriendResponse(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();
    bool flag = js["flag"].get<bool>();
    json response;
    response["msgid"] = ADD_FRIEND_MSG_ACK;
    response["from"] = userid;
    response["toid"] = friendid;
    if(flag) {
        response["msg"] = "I very happy to make friends with you!!!";
        ConnPool* connPool = this->getConnPool();
        m_friendModel.insert(connPool,userid, friendid);
    }
    else{
        response["msg"] = "I am very sorry, you are not my friend!!!";
    }
    cout<<"response.dump() : "<<response.dump()<<endl;
    oneChat(conn,response,time);
}

// 获取消息msgid对应的处理器
MsgHandler ChatService::getHandler(int msgid) {
    // 记录错误日志,msgid没有对应的事件处理回调
    auto it = m_msgHandlerMap.find(msgid);
    if(it == m_msgHandlerMap.end()) {
        // 返回一个默认的处理器,空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp) {
            LOG_ERROR << "msgid:" << msgid << " can not find handler!";
        };//msgid没有对应处理器，打印日志，返回一个默认处理器，空操作
    }
    else {
        return m_msgHandlerMap[msgid];
    }
}

// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn) {
    User user;
    {
        lock_guard<mutex> lock(m_connMutex);   
        // 1.从map表删除用户的连接信息
        for(auto it = m_userConnMap.begin();it!=m_userConnMap.end();++it) {
            if(it->second == conn) {
                // 从map表删除用户的链接信息
                user.setId(it->first);
                m_userConnMap.erase(it);
                break;
            }
        }
    }

    // 用户注销,相当于就是下线,在redis中取消订阅通道
    m_redis.unsubscribe(user.getId());

    // 2.更新用户的状态信息
    if(user.getId() != -1) {
        user.setState("offline");
        ConnPool* connPool = this->getConnPool();
        m_userModel.updateState(connPool,user);
    }
   
}

// 服务器异常，业务重置方法
void ChatService::reset() {
    // 把online状态的用户,设置成offline
    ConnPool* connPool = this->getConnPool();
    m_userModel.resetState(connPool);
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    // 1.获取创建群的用户id,群名称,群功能
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];
    // 2.存储新创建的群组信息
    ConnPool* connPool = this->getConnPool();
    Group group(-1, name, desc);
    if(m_groupModel.createGroup(connPool,group)) {
        // 存储群组创建人信息
        m_groupModel.joinGroup(connPool,userid,group.getId(),"creator");
    }
}

// 加入群组业务
void ChatService::joinGroup(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    // 存储用户加入的群组信息
    ConnPool* connPool = this->getConnPool();
    m_groupModel.joinGroup(connPool,userid,groupid,"normal");
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    // 1.获取要发送消息的用户id,要发送的群组id
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    // 2.查询该群组其他的用户id
    ConnPool* connPool = this->getConnPool();
    vector<int> useridVec = m_groupModel.queryGroupUsers(connPool,userid, groupid);  
    
    // 3.进行用户查找
    /*
     * A向B说话,在map表中未找到B,B可能不在本台服务器上但通过数据库查找
     * 在线,要发送的消息直接发送以B用户为id的通道上;也可能是离线状态,
     * 发送离线消息
     */
    lock_guard<mutex> lock(m_connMutex);
    for(int id : useridVec) {
        auto it = m_userConnMap.find(id);
        // 用户在线，转发群消息
        if(it != m_userConnMap.end()) {
            // 转发群消息
            it->second->send(js.dump());
        }
        else {  // 用户不在线,存储离线消息 或 在其它服务器上登录的
            // 查询toid是否在线
            User user = m_userModel.query(connPool,id);
            if(user.getState() == "online") { // 在其他服务器上登录的
                m_redis.publish(id,js.dump());
            }else{
                // 存储离线群消息
                ConnPool* connPool = this->getConnPool();
                m_offlineMsgModel.insert(connPool,id, js.dump());
            }
        }
    }
}

//处理注销业务
void ChatService::loginOut(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    //1、获取要注销用户的id，删除对应连接
    int userid = js["id"].get<int>();
    // std::cout<<"获取要注销用户的id，删除对应连接: userid: "<<userid<<std::endl;
    {
        lock_guard<mutex> lock(m_connMutex);
        auto it = m_userConnMap.find(userid);
        if (it != m_userConnMap.end())
        {
            m_userConnMap.erase(it);
        }
    }

    // 用户注销,相当于就是下线,在redis中取消订阅通道
    m_redis.unsubscribe(userid);

    //2、更新用户状态信息
    User user(userid, "", "", "offline");
    ConnPool* connPool = this->getConnPool();
    m_userModel.updateState(connPool,user);
}

// 从redis消息队列中获取订阅的消息：通道号 + 消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg) {
    lock_guard<mutex> lock(m_connMutex);
    auto it = m_userConnMap.find(userid);
    if (it != m_userConnMap.end()) {
        it->second->send(msg);
        return;
    }
    // 存储该用户的离线消息:在从通道取消息时,用户下线则发送离线消息
    ConnPool* connPool = this->getConnPool();
    m_offlineMsgModel.insert(connPool,userid, msg);
}

/*
服务器业务模块ChatService
服务器业务模块:客户端发送的业务数据,先到达服务器端网络模块,
网络模块进行事件分发到业务模块相应的业务处理器,最终通过数据
层访问底层数据模块

3.1 用户注册业务
用户注册:服务器将客户端收到的json反序列化后存储到数据库中,依据是否
注册成功给客户端返回响应消息
*/