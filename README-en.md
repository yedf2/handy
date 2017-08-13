handy[![Build Status](https://travis-ci.org/yedf/handy.png)](https://travis-ci.org/yedf/handy)
====
[中文版](https://github.com/yedf/handy/blob/master/README.md)
## A C++11 non-blocking network library

### multi platform support

*   Linux: ubuntu14 64bit g++4.8.1 tested

*   MacOSX: LLVM version 6.1.0 tested

### elegant program exit

programmer can write operations for exit

can use valgrind to check memory leak

### high performance

*   use epoll on Linux

*   use kqueue on MacOSX

[performance report](http://www.oschina.net/p/c11-handy)

### elegant

only 10 lines can finish a complete server

### sample --echo-server

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

### half sync half async pattern

processing I/O asynchronously and Request synchronously can greatly simplify the coding of business processing

example can be found examples/hsha.cc

### openssl supported

asynchronously handle the openssl connection. if you have installed openssl, then make will automatically download handy-ssl.
ssl support files are in [handy-ssl](https://github.com/yedf/handy-ssl.git) because of license.

###protobuf supported

examples can be found in directory protobuf

###Installation

    make && make install

###contents

*   handy--------handy library  
*   examples---- 
*   ssl------------openssl support and examples 
*   protobuf-----protobuf support and examples
*   test-----------handy test case  

###[hand book](https://github.com/yedf/handy/blob/master/doc-cn.md)

license
====
Use of this source code is governed by a BSD-style
license that can be found in the License file.

email
====
dongfuye@163.com
