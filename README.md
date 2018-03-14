handy[![Build Status](https://travis-ci.org/yedf/handy.png)](https://travis-ci.org/yedf/handy)
====
[English](https://github.com/yedf/handy/blob/master/README-en.md)

## 简洁易用的C++11网络库

### 多平台支持

*   Linux: ubuntu14 64bit g++4.8.1 上测试通过

*   MacOSX: LLVM version 6.1.0 上测试通过

*   MacOSX: 支持CLion IDE

### 支持优雅退出

优雅退出可以让程序员更好的定义自己程序的退出行为

能够更好的借助valgrind等工具检查内存泄露。

### 高性能

*   linux上使用epoll
*   MacOSX上使用kqueue
*   [性能测试报告](http://www.oschina.net/p/c11-handy)
*   [单机千万并发连接](https://zhuanlan.zhihu.com/p/21378825)

### 简洁

10行代码能够编写一个完整的服务器

### 代码示例--echo-server

```c
#include <handy/handy.h>
using namespace handy;

int main(int argc, const char* argv[]) {
    EventBase base;
    Signal::signal(SIGINT, [&]{ base.exit(); });
    TcpServerPtr svr = TcpServer::startServer(&base, "", 2099);
    exitif(svr == NULL, "start tcp server failed");
    svr->onConnRead([](const TcpConnPtr& con) {
        con->send(con->getInput());
    });
    base.loop();
}
```

### 支持半同步半异步处理

异步管理网络I/O，同步处理请求，可以简化服务器处理逻辑的编写，示例参见examples/hsha.cc

### openssl支持

异步连接管理，支持openssl连接，如果实现安装了openssl，能够找到<openssl/ssl.h>，项目会自动下载handy-ssl
由于openssl的开源协议与此不兼容，所以项目文件单独放在[handy-ssl](https://github.com/yedf/handy-ssl.git)

### protobuf支持

使用protobuf的消息encode/decode示例在protobuf下

### udp支持

支持udp，udp的客户端采用connect方式使用，类似tcp

### 安装与使用

    make && make install

### 目录结构

*   handy--------handy库
*   10m----------进行千万并发连接测试所使用的程序
*   examples----示例
*   raw-examples--原生api使用示例，包括了epoll，epoll ET模式，kqueue示例
*   ssl------------openssl相关的代码与示例  
*   protobuf-----handy使用protobuf的示例  
*   test-----------handy相关的测试  

### [使用文档](https://github.com/yedf/handy/blob/master/doc.md)

### raw-examples
使用os提供的api如epoll，kqueue编写并发应用程序
*   epoll.cc，演示了epoll的通常用法，使用epoll的LT模式
*   epoll-et.cc，演示了epoll的ET模式，与LT模式非常像，区别主要体现在不需要手动开关EPOLLOUT事件

### examples
使用handy的示例
*   echo.cc 简单的回显服务
*   timer.cc 使用定时器来管理定时任务
*   idle-close.cc 关闭一个空闲的连接
*   reconnect.cc 设置连接关闭后自动重连
*   safe-close.cc 在其他线程中安全操作连接
*   chat.cc 简单的聊天应用，用户使用telnet登陆后，系统分配一个用户id，用户可以发送消息给某个用户，也可以发送消息给所有用户
*   codec-cli.cc 发送消息给服务器，使用的消息格式为mBdT开始，紧接着4字节的长度，然后是消息内容
*   codec-svr.cc 见上
*   hsha.cc 半同步半异步示例，用户可以把IO交给handy框架进行处理，自己同步处理用户请求
*   http-hello.cc 一个简单的http服务器程序
*   stat.cc 一个简单的状态服务器示例，一个内嵌的http服务器，方便外部的工具查看应用程序的状态
*   write-on-empty.cc 这个例子演示了需要写出大量数据，例如1G文件这种情景中的使用技巧
*   daemon.cc 程序已以daemon方式启动，从conf文件中获取日志相关的配置，并初始化日志参数
*   udp-cli.cc udp的客户端
*   udp-svr.cc udp服务器
*   udp-hsha.cc udp的半同步半异步服务器

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

如果您觉得此项目不错，或者对您有帮助，请赏颗星吧！
