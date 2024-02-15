#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>
using namespace std;

class Redis {
public:
    Redis();
    ~Redis();
    // 连接redis服务器
    bool connect();
    // 向redis指定的通道channel发布消息
    bool publish(int channel,string message);
    // 向redis指定的通道subscribe订阅消息
    bool subscribe(int channel);
    // 向redis指定的通道unsubscribe取消订阅消息
    bool unsubscribe(int channel);
    // 在独立线程中接收订阅通道中的消息
    void observer_channel_message();
    // 初始化向业务层上报通道消息的回调对象
    void init_notify_handler(function<void(int,string)> fn);
private:
    // hiredis同步上下文对象,负责publish消息:相当于我们客户端一个redis-cli跟连接相关的所有信息，需要两个上下文处理
    redisContext* m_publish_context;
    // hiredis同步上下文对象,负责subscribe消息
    redisContext* m_subscribe_context;
    // 回调操作,收到订阅的消息,给service层上报:主要上报通道号、数据
    function<void(int,string)>m_notify_message_handler;
};
#endif