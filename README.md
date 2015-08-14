handy[![Build Status](https://travis-ci.org/yedf/handy.png)](https://travis-ci.org/yedf/handy)
====
[English](https://github.com/yedf/handy/blob/master/README-en.md)
##简洁易用的C++11网络库--参考了陈硕的[muduo](http://github.com/chenshuo/muduo/)

###多平台支持

*   Linux: ubuntu14 64bit g++4.8.1 上测试通过

*   MacOSX: LLVM version 6.1.0 上测试通过

###支持优雅退出

优雅退出可以让程序员更好的定义自己程序的退出行为

能够更好的借助valrind等工具检查内存泄露。

###高性能

*   linux上使用epoll

*   MacOSX上使用kqueue

[性能测试报告](http://www.oschina.net/p/c11-handy)

###简洁

10行代码能够编写一个完整的服务器

###代码示例--echo-server

```c
#include <handy/handy.h>

using namespace std;
using namespace handy;


int main(int argc, const char* argv[]) {
    EventBase bases; //事件分发器
    Signal::signal(SIGINT, [&]{ bases.exit(); }); //注册Ctrl+C的信号处理器--退出事件分发循环
    TcpServer echo(&bases); //创建服务器
    int r = echo.bind("", 99); //绑定端口
    exitif(r, "bind failed %d %s", errno, strerror(errno));
    echo.onConnRead([](const TcpConnPtr& con) {
        con->send(con->getInput()); // echo 读取的数据
    });
    bases.loop(); //进入事件分发循环
}
```

###支持半同步半异步处理

异步管理网络I/O，同步处理请求，可以简化服务器处理逻辑的编写，示例参见examples/hsha.cc

###openssl支持

异步连接管理，支持openssl连接

###protobuf支持

使用protobuf的消息encode/decode示例在protobuf下

###安装与使用

    make && make install

###目录结构

*   handy--------handy库  
*   examples----示例  
*   ssl------------openssl相关的代码与示例  
*   protobuf-----handy使用protobuf的示例  
*   test-----------handy相关的测试  

###[使用文档](https://github.com/yedf/handy/blob/master/doc.md)

license
====
Use of this source code is governed by a BSD-style
license that can be found in the License file.

email
====
dongfuye@163.com

qq群
====
189076978