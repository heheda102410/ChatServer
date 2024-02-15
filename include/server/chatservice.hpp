#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>

using namespace std;
using namespace muduo;
using namespace muduo::net;

#include "json.hpp"
using json = nlohmann::json;

#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"

#include "redis.hpp"
#include "ConnPool.h"

// 表示处理消息的事件回调方法类型
using MsgHandler = std::function<void(const TcpConnectionPtr& conn,json& js,Timestamp)>;

// 聊天服务器业务类,设计为单例模式:给msgid映射事件回调(一个消息id映射一个事件处理)
class ChatService {
public:
    // 获取单例对象的接口函数
    static ChatService* getInstance();
    // 处理登录业务
    void login(const TcpConnectionPtr& conn,json& js,Timestamp time); 
    // 处理注册业务(register)
    void reg(const TcpConnectionPtr& conn,json& js,Timestamp time); 
    // 处理一对一聊天业务
    void oneChat(const TcpConnectionPtr& conn,json& js,Timestamp time);
    // 添加好友业务
    // void addFriend(const TcpConnectionPtr& conn,json& js,Timestamp time);
    // 添加好友业务请求
    void addFriendRequest(const TcpConnectionPtr& conn,json& js,Timestamp time);
    // 添加好友业务响应
    void addFriendResponse(const TcpConnectionPtr& conn,json& js,Timestamp time);

    // 获取消息msgid对应的处理器
    MsgHandler getHandler(int msgid);
    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr& conn);
    // 服务器异常，业务重置方法
    void reset();
    // 创建群组业务
    void createGroup(const TcpConnectionPtr& conn,json& js,Timestamp time); 
    // 加入群组业务
    void joinGroup(const TcpConnectionPtr& conn,json& js,Timestamp time);   
    // 群组聊天业务
    void groupChat(const TcpConnectionPtr& conn,json& js,Timestamp time); 
    // 处理注销业务
    void loginOut(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 从redis消息队列中获取订阅的消息：通道号 + 消息
    void handleRedisSubscribeMessage(int userid, string msg);

    ChatService(const ChatService&) = delete;
    ChatService& operator=(const ChatService&) = delete;

    ConnPool* getConnPool() const { return m_connPool;}
private:
    // 注册消息以及对应的Handler回调操作
    ChatService();
    // 存储消息id和其对应的业务处理方法
    unordered_map<int,MsgHandler> m_msgHandlerMap;
    // 存储在线用户的通信连接
    unordered_map<int,TcpConnectionPtr> m_userConnMap; // 消息处理器map表 每一个msgid对应一个业务处理方法
    // 定义互斥锁,保证m_userConnMap的线程安全
    mutex m_connMutex;
    // 数据操作类对象
    UserModel m_userModel;              // 存储在线用户的通信连接map表
    OfflineMsgModel m_offlineMsgModel;  // 离线消息表的数据操作类对象
    FriendModel m_friendModel;          // 好友表的数据操作类对象
    GroupModel m_groupModel;
    Redis m_redis;                      // redis操作对象
    ConnPool* m_connPool;                   // 数据库连接池
};

#endif // CHATSERVICE_H

/*
3.1 用户注册业务:
    我们业务层与数据层分离,需要操作数据层数据对象即可,因此需要在
    ChatService类中实例化一个数据操作类对象进行业务开发
    UserModel m_userModel;// 数据操作类对象

服务器注册业务流程:
1.客户端注册的消息过来后,网络模块将json数据反序列化后上报到注册业务中,
因为User表中id字段为自增的,state字段是默认的,因此注册业务只需要获取
name与password字段即可
2.实例化User表对应的对象user,将获取到的name与password设置进去,再向
UserModel数据操作类对象进行新用户user的注册
3.注册完成后,服务器返回相应json数据给客户端:若注册成功,返回注册响应消息
REG_MSG_ACK,错误标识errno(0:成功,1:失败),用户id等组装好的json数据;
若注册失败,返回注册响应消息REG_MSG_ACK,错误标识

3.2 用户登录业务
3.2.1 基础登录业务实现
用户登录:服务器反序列化数据后,依据id,密码字段后判断账号是否正确,依据是否
登陆成功给客户端返回响应消息
服务器登录业务流程:
1.服务器获取输入用户id,密码字段
2.查询id对应的数据,判断用户id与密码是否正确,分为以下三种情况返回相应json数据给客户端:
(1)若用户名/密码正确且未重复登录,及时更新登录状态为在线,，返回登录响应消息
   LOGIN_MSG_ACK,错误标识errno(0:成功,1:失败,2:重复登录),用户id,用户名等信息
(2)若用户名/密码正确但重复登录,返回登录响应消息、错误标识、错误提示信息；
(3)若用户不存在或密码错误,返回登录响应消息,错误标识,错误提示信息;

3.2.2 记录用户连接信息处理
用户连接信息处理:假设此时用户1向用户2发送消息(源id, 目的id,消息内容),
此时服务器收到用户1的数据了,要主动向用户2推送该条消息,那么如何知道用户2
是那条连接呢。因此我们需要专门处理下,用户一旦登录成功,就会建立一条连接,
我们便要将该条连接存储下来,方便后续消息收发的处理.

3.2.3 客户端异常退出处理
客户端异常退出处理:假设用户客户端直接通过Ctrl+C中断,并没有给服务器发送合法的json过来，
我们必须及时修改用户登录状态，否则后续再想登录时为"online"状态，便无法登录了。

客户端异常退出处理流程:
1.通过conn连接去m_userConnMap表中查找,删除conn键值对记录;
2.将conn连接对应用户数据库的状态从"online"改为"offline";

3.2.4 服务器异常退出处理
服务器异常退出处理:假设用户服务器直接通过Ctrl+C中断,并没有给客户端发送
合法的json过去,我们必须及时修改所有用户登录状态未"offline",否则后续再
想登录时为"online"状态，便无法登录了。
服务器异常退出处理流程:主动截获Ctcl+c信号(SIGINT),在信号处理函数中将
数据库中用户状态重置为"offline"。

3.3 点对点聊天业务
点对点聊天:源用户向目的用户发送消息,目的用户若在线则将消息发出,
目的用户若不在线将消息存储至离线消息表中,待目的用户上线后离线
消息发出

在进行点对点聊天业务处理前,需要提前处理好以下几点:
在EnMsgType中增加一个聊天消息类型,给客户端标识此时是一个聊天消息.
将点对点业务的消息id与对应的事件处理器提前在聊天服务器业务类的构造
函数里绑定好

服务器点对点聊天业务流程
1.源id向目的id发送消息时候,消息里会包含消息类型,源id,源用户名,
目的id,消息内容,服务器解析到这些数据后,先获取到目的id字段
2.找到id判断是否在线,若在线则服务器将源id的消息中转给目的id;若
不在线则将消息内容存入离线消息表中,待目的id上线后离线消息发出

3.4 离线消息业务
离线消息业务:当用户一旦登录成功,我们查询用户是否有离线消息要发送,
若有则发送相应数据,发送完后删除本次存储的离线数据,防止数据重复发送
在进行点对点聊天业务处理前,我们需要提前处理好以下几点:
1、建立与离线消息表的映射OfflineMsgModel类：我们数据库中有创建的
OfflineMessage离线消息表，因为我们数据层与业务层要分离开来，所以
这里与前面一样提供离线消息表的数据操作类，提供给业务层对应的操作接口。

服务器离线消息业务流程:
1.无论是一对一聊天,还是群聊,若接收方用户不在线,则将发送方消息先存储至离线消息表里
2.一旦接收方用户登录成功,检查该用户是否有离线消息(可能有多条),若有则服务器
将离线消息发送给接收方用户
3.服务器发送完成后删除本次存储的离线消息,保证接收方不会每次登录都收到重复的离线消息

3.5 添加好友业务
添加好友业务:源用户id、目的用户id发送给服务器，服务器在数据库中进行好友关系的添加。
添加完成用户登录后，服务器返回好友列表信息给用户，用户可以依据好友列表进行聊天，这里实现的比较简单，后续可扩充更细化的业务。
在进行添加好友业务处理前，我们需要提前处理好以下几点：
1、我们需要在消息类型EnMsgType中增加一个聊天消息类型，给客户端标识此时是一个添加好友消息：
2、将添加好友业务的消息id与对应的事件处理器提前在聊天服务器业务类的构造函数里绑定好。
3、建立好友表与类的映射FriendModel类：表中userid与friendid关系只需要存储一次即可，因此为联合主键。这里与前面一样提供好友表的数据操作类，提供给业务层对应的操作接口。

服务器添加好友业务流程:
1.服务器获取当前用户id,要添加好友的id;
2.业务层调用数据层接口往数据库中添加相应好友信息;
用户登录成功时,查询该用户的好友信息并返回

3.6 群组业务
群组业务:群组业务分为三块,群管理员创建群组,组员加入群组与群组聊天功能
在进行群组业务处理前,我们需要提前处理好以下几点:
1.我们需要在消息类型EnMsgType中增加不同的消息类型,创建群组,
加入群组、群组聊天三种类型消息，给客户端标识此时要做什么事情：

3.6.1 创建群组
服务器创建群组业务,业务流程:
1.服务器获取创建群的用户id,要创建群名称,群功能等信息
2.业务层创建数据层对象,调用数据层方法进行群组创建，创建成功保存群组创建人信息；

3.6.2 加入群组
服务器组员加入群组业务流程:
1、服务器获取要加入群用户的id、要加入的群组id；
2、业务层调用数据层方法将普通用户加入；

3.6.3 群组聊天
服务器群组聊天业务流程:
1、获取要发送消息的用户id、要发送的群组id；
2、查询该群组其它用户id；
3、查询同组用户id，若用户在线则发送消息；若用户不在线则存储离线消息；

3.7 注销业务
注销业务： 客户端用户正常退出，更新其在线状态。

在进行注销业务处理前，我们需要提前处理好以下几点：
1、我们需要在消息类型EnMsgType中增加一个注销业务类型，给客户端标识此时是一个注销业务消息：
2、将注销业务的消息id与对应的事件处理器提前在聊天服务器业务类的构造函数里绑定好。

服务器注销业务业务流程：
1、服务器获取要注销用户的id，删除其对应的连接。
2、更新用户状态信息，从在线更新为离线。


四 服务器支持跨服务器通信功能
redis主要业务流程:
1.用户登录成功后相应的服务器需要向redis上依据用户id订阅相应通道的消息
2.当服务器上用户之间跨服务器发送消息时，需要向通道上发送消息
3、redis接收到消息通知相应服务器进行处理
*/