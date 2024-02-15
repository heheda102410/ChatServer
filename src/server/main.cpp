#include "chatserver.hpp"
#include "chatservice.hpp"
#include <iostream>
#include <signal.h>
using namespace std;

// 处理服务器ctrl+c结束后,重置user的状态信息
void resetHandler(int) {
    ChatService::getInstance()->reset();
    exit(0);
}

int main(int argc, char** argv) {
    signal(SIGINT,resetHandler);
    
    // InetAddress addr("127.0.0.1", 6000);
    char* ip = argv[1];
    uint16_t port = atoi(argv[2]);
    InetAddress addr(ip, port);
    
    EventLoop loop;
    ChatServer server(&loop, addr, "ChatServer");
    server.start();
    loop.loop(); // 启动事件循环
    return 0;
}