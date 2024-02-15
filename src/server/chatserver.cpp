#include "chatserver.hpp"
#include "chatservice.hpp"
#include "json.hpp"
#include <functional>
#include <string>
#include <iostream>
using namespace std;
using namespace placeholders;
using json = nlohmann::json;

// 初始化聊天服务器对象
ChatServer::ChatServer(EventLoop *loop, const InetAddress &listenAddr, const string &nameArg)
    : m_server(loop, listenAddr, nameArg), m_loop(loop) {
    // 注册用户连接的创建和断开事件的回调
    m_server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
    // 注册用户读写事件的回调
    m_server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
    // 设置服务器线程数量 1个I/O线程,3个工作线程
    m_server.setThreadNum(4);
}

// 启动服务,开启事件循环
void ChatServer::start() {
    m_server.start();
}

// 上报链接相关信息的回调函数:参数为连接信息
void ChatServer::onConnection(const TcpConnectionPtr &conn) {
    // 客户端断开连接,释放连接资源 muduo库会打印相应日志
    if(!conn->connected()) {
        ChatService::getInstance()->clientCloseException(conn);// 处理客户端异常关闭
        conn->shutdown();// 释放socket fd资源
    }
}

// 网络模块与业务模块解耦:不直接调用相应方法,业务发生变化此处代码也不需要改动
// 上报读写事件相关信息的回调函数:参数分别为连接/缓冲区/接收到数据的时间信息
void ChatServer::onMessage(const TcpConnectionPtr &conn, Buffer *buffer, Timestamp time) {
    // 将buffer缓冲区收到的数据存入字符串
    string buf = buffer->retrieveAllAsString();
    
    std::cout<<"buf: "<<buf.c_str()<<std::endl;
    // 数据的反序列化
    json js = json::parse(buf);
    // 达到的目的:完全解耦网络模块的代码和业务模块的代码
    // 通过js["msgid"] 获取 => 业务handler => conn js time
    auto msghandler = ChatService::getInstance()->getHandler(js["msgid"].get<int>());
    // 回调消息绑定好的事件处理器,来执行相应的业务处理
    msghandler(conn,js,time);
}
