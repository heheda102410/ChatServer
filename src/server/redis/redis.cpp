#include <iostream>
using namespace std;
#include "redis.hpp"
//构造函数:初始化两个上下文指针
Redis::Redis() 
    : m_publish_context(nullptr)
    , m_subscribe_context(nullptr)
{
}

//析构函数:释放两个上下文指针占用资源
Redis::~Redis() {
    if (m_publish_context != nullptr) {
        redisFree(m_publish_context);
        // m_publish_context = nullptr;
    }

    if (m_subscribe_context != nullptr) {
        redisFree(m_subscribe_context);
        // m_subscribe_context = nullptr;
    }
}

//连接redis服务器
bool Redis::connect() {
    //负责publish发布消息的上下文连接
    m_publish_context = redisConnect("127.0.0.1", 6379);
    if (nullptr == m_publish_context) {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    //负责subscribe订阅消息的上下文连接
    m_subscribe_context = redisConnect("127.0.0.1", 6379);
    if (nullptr == m_subscribe_context) {
        cerr << "connect redis failes!" << endl;
        return false;
    }

    //在单独的线程中监听通道上的事件，有消息给业务层上报 让线程阻塞去监听
    thread t([&](){
        observer_channel_message();
    });
    t.detach();

    cout << "connect redis-server success!" << endl;

    return true;
}

//向redis指定的通道channel publish发布消息:调用redisCommand发送命令即可
bool Redis::publish(int channel, string message) {
    redisReply *reply = (redisReply *)redisCommand(m_publish_context, "PUBLISH %d %s", channel, message.c_str()); //相当于给channel通道发送消息
    if (nullptr == reply) {
        cerr << "publish command failed!" << endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

/* 为什么发布消息使用redisCommand函数即可，而订阅消息却不使用？
redisCommand本身会先调用redisAppendCommand将要发送的命令缓存到本地，再调用redisBufferWrite将命令发送到redis服务器上，再调用redisReply以阻塞的方式等待命令的执行。
subscribe会以阻塞的方式等待发送消息，线程是有限，每次订阅一个线程会导致线程阻塞住，这肯定是不行的。
publish一执行马上会回复，不会阻塞当前线程，因此调用redisCommand函数。
*/

//向redis指定的通道subscribe订阅消息:
bool Redis::subscribe(int channel) {
    // SUBSCRIBE命令本身会造成线程阻塞等待通道里面发生消息，这里只做订阅通道，不接收通道消息
    // 通道消息的接收专门在observer_channel_message函数中的独立线程中进行
    // 只负责发送命令，不阻塞接收redis server响应消息，否则和notifyMsg线程抢占响应资源
    if (REDIS_ERR == redisAppendCommand(this->m_subscribe_context, "SUBSCRIBE %d", channel)) { //组装命令写入本地缓存
        cerr << "subscribe command failed!" << endl;
        return false;
    }
    
    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done) {
        if (REDIS_ERR == redisBufferWrite(this->m_subscribe_context, &done)) { //将本地缓存发送到redis服务器上
            cerr << "subscribe command failed!" << endl;
            return false;
        }
    }
    // redisGetReply

    return true;
}

//向redis指定的通道unsubscribe取消订阅消息，与subscrible一样
bool Redis::unsubscribe(int channel) {
    if (REDIS_ERR == redisAppendCommand(this->m_subscribe_context, "UNSUBSCRIBE %d", channel)) {
        cerr << "unsubscribe command failed!" << endl;
        return false;
    }
    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done) {
        if (REDIS_ERR == redisBufferWrite(this->m_subscribe_context, &done)) {
            cerr << "unsubscribe command failed!" << endl;
            return false;
        }
    }
    return true;
}

//在独立线程中接收订阅通道中的消息:以循环阻塞的方式等待响应通道上发生消息
void Redis::observer_channel_message() {
    redisReply *reply = nullptr;
    while (REDIS_OK == redisGetReply(this->m_subscribe_context, (void**)&reply)) {
        //订阅收到的消息是一个带三元素的数，通道上发送消息会返回三个数据，数据下标为2
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr) {
            //给业务层上报通道上发送的消息:通道号、数据
            m_notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
}

//初始化向业务层上报通道消息的回调对象
void Redis::init_notify_handler(function<void(int, string)> fn) {
    this->m_notify_message_handler = fn;
}
