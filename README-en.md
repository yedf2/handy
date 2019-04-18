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

## Usage

### Quick start
```
 make && make install
```

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

### protobuf supported

examples can be found in directory protobuf

### contents

*   handy--------handy library  
*   examples---- 
*   ssl------------openssl support and examples 
*   protobuf-----protobuf support and examples
*   test-----------handy test case  

### [hand book](https://github.com/yedf/handy/blob/master/doc-cn.md)

## Advanced build option

### Build handy shared library and examples:
```
$ git clone https://github.com/yedf/handy
$ cd handy && mkdir build && cd build
$ cmake -DBUILD_HANDY_SHARED_LIBRARY=ON -DBUILD_HANDY_EXAMPLES=ON -DCMAKE_INSTALL_PREFIX=/tmp/handy ..
$ make -j4
$ make install
$ ls /tmp/handy
bin  include  lib64
$ ls /tmp/handy/bin/
10m-cli  10m-svr  codec-cli  codec-svr  daemon  echo  hsha  http-hello  idle-close  reconnect  safe-close  stat  timer  udp-cli  udp-hsha  udp-svr  write-on-empty
$ ls /tmp/handy/lib64/
libhandy_s.a  libhandy.so
```

### As a static library in your own programs:
* add handy as a git submodule to say a folder called vendor
* in your CMakeLists.txt

```
add_subdirectory("vendor/handy" EXCLUDE_FROM_ALL)

add_executable(${PROJECT_NAME} main.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC
    "vendor/handy"
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    handy_s
)
```

license
====
Use of this source code is governed by a BSD-style
license that can be found in the License file.

email
====
dongfuye@163.com
