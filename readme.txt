基于muduo的客户端服务器编程

muduo网络库的编程很容易,要实现基于muduo网络库的服务器和客户端程序,只需要简单的组合TcpServer和TcpClient就可以,代码实现如下:

CMake常用的预定义变量

PROJECT_NAME:通过project()指定项目名称

PROJECT_SOURCE_DIR 工程的根目录

PROJECT_BINARY_DIR 执行cmake命令的目录

CMAKE_CURRENT_SOURCE_DIR 当前处理的CMakeLists.txt所在的目录

CMAKE_CURRENT_BINARY_DIR 编译目录，可使用add add_subdirectory来修改

EXECUTABLE_OUTPUT_PATH 重新定义目标二进制可执行文件的存放位置

LIBRARY_OUTPUT_PATH 重新定义目标链接库文件的存放位置

BUILD_SHARED_LIBS 默认的库编译方式（shared 或 static）,默认为static

CMAKE_C_FLAGS 设置C编译选项

CMAKE_CXX_FLAGS 设置C++编译选项

CMAKE_CXX_FLAGS_DEBUG 设置C++ Debug编译选项

CMAKE_CXX_FLAGS_RELEASE 设置C++ Release编译选项

CMAKE_GENERATOR 指定CMake生成器，比如Unix Makefiles、MinGW Makefiles、NMake Makefiles、Visual Studio 10 2010、Visual Studio 12 2013、Visual Studio 14 2015、Visual Studio 15 2017、Xcode等

CMAKE_COMMAND CMake可执行文件本身的全路径

CMAKE_BUILD_TYPE 工程编译生成的版本，Debug或Release

update mysql.user set authentication_string=password('123456') where user='root' and host='localhost';

update mysql.user set plugin='mysql_native_password';

flush privileges;

quit;

/*

    json里边会包含一个msgid.由于客户端和服务器通信收发消息,

    需要判断这个消息是属于哪种业务的,就需要一个业务的标识,所以

    就用msgid来表示业务的标识.在onMessage函数中,并不想出现

    当有登录业务需求就调用相应的服务登录方法,当有注册业务需求就

    调用相应的服务注册方法,这样就用到if...else,或者switch case,

    但这种方式是直接调用服务层的方法,就把网络模块的代码和业务模块的

    代码给强耦合一起了,这不是好的方法.

    方法二:每一个消息都有一个msgid(一个消息id映射一个事件处理),

    事先给它绑定一个回调操作,让一个id对应一个操作.不管具体做什么业务,

    并不会直接调用业务模块的相关的方法.

    利用OOP回调思想,要想解耦模块之间的关系,一般有两种方法,一种就是

    使用基于面向接口的编程,在C++里边的"接口"可以理解为抽象基类.那也就是

    面向抽象基类的编程.另一种就是基于回调函数

    这里使用基于回调函数来实现,m_msgHandlerMap存储消息id和其对应的业务处理方法.

    注册消息以及对应的Handler回调操作,就是把消息id对应的事件处理器给绑定了,LOGIN_MSG

    绑定的是login处理登录业务,REG_MSG绑定的是reg处理注册业务.

    ChatService单例对象通过js["msgid"] 获取消息对应的处理器(业务handler)msghandler,由于

    回调消息绑定了事件处理器,可用它来执行相应的业务处理-->msghandler(conn,js,time);

*/

heheda@linux:~/Linux/Server$ sudo find /usr -name libmysqlclient*
[sudo] heheda 的密码：
/usr/lib/x86_64-linux-gnu/libmysqlclient.so.20
/usr/lib/x86_64-linux-gnu/libmysqlclient.a
/usr/lib/x86_64-linux-gnu/libmysqlclient.so.20.3.29
/usr/lib/x86_64-linux-gnu/libmysqlclient.so
/usr/share/doc/libmysqlclient20
/usr/share/doc/libmysqlclient-dev



客户端发送过来一个注册的业务，先从最开始的网络，再通过事件的分发，

到业务层的相关的handler处理注册，接着访问底层的model。其中在业务类设计

这里看到的都是对象，方便你把底层的数据模块改成你想要的，例如mysql,sql,oracle,mongoDB等都行。实现了网络模块，业务模块以及数据模块的

低耦合。


{"msgid":3,"name":"heheda","password":"1024"} // 注册
{"msgid":3,"name":"Tom","password":"520"} // 注册
{"msgid":3,"name":"Jerry","password":"1314"} // 注册

{"msgid":1,"id":"1","password":"123456"} // 错误

{"msgid":1,"id":2,"password":"520"}  // 登录
{"msgid":1,"id":10,"password":"1314"} 


heheda@linux:~/Linux/Server$ gdb ./bin/ChatServer
(gdb) break chatservice.cpp 20
(gdb) run
(gdb) n

buf: {"msgid":1,"id":"1","password":"123456"}

exception caught in Thread ChatServer0
reason: [json.exception.type_error.302] type must be number, but is string

truncate user;

一对一聊天的json消息
msgid
id:1
from:"zhang san"
to:3
msg:"xxxxxxx"


{"msgid":3,"name":"heheda","password":"1024"} // 注册
{"msgid":3,"name":"Tom","password":"520"} // 注册
{"msgid":3,"name":"Jerry","password":"1314"} // 注册

{"msgid":1,"id":1,"password":"1024"}  // 登录
{"msgid":1,"id":2,"password":"520"}  // 登录
{"msgid":1,"id":3,"password":"1314"}  // 登录

{"msgid":5,"id":1,"from":"heheda","to":2,"msg":"hello,I am Heheda!asasa"}
{"msgid":5,"id":2,"from":"Tom","to":1,"msg":"hello,I am Tom!"}

{"msgid":6,"id":1,"friendid":2}

mysql> desc offlinemessage;
+---------+--------------+------+-----+---------+-------+
| Field   | Type         | Null | Key | Default | Extra |
+---------+--------------+------+-----+---------+-------+
| userid  | int(11)      | NO   | PRI | NULL    |       |
| message | varchar(500) | NO   |     | NULL    |       |
+---------+--------------+------+-----+---------+-------+
2 rows in set (0.02 sec)

mysql>update user set state='offline' 
