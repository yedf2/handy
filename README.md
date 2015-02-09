handy
====

a simple C++11 network library/简洁易用的C++11网络库--linux

reactor pattern/reactor 模式

elegantly exit/支持优雅退出

lock free log system, rotation supported/无锁日志系统，按时间间隔轮替

use C++11 to simplify code, similar as muduo/参考muduo的实现，采用C++11简化代码

tested on ubuntu14 64bit g++4.8.1/ubuntu14 64位 g++ 4.8.1上通过测试

performance
====
see [http://www.oschina.net/p/c11-handy](http://www.oschina.net/p/c11-handy)

install
====
./build_config && make

examples
====
<pre><code>
#include "conn.h"
#include "daemon.h"

using namespace std;
using namespace handy;

int main(int argc, const char* argv[]) {
    EventBase base;
    Signal::signal(SIGINT, [&]{ base.exit(); });

    TcpServer echo(&base, Ip4Addr(99));
    echo.onConnRead(
        [](const TcpConnPtr& con) { 
            con->send(con->getInput());
        }
    );
    base.loop();
}
</code></pre>
license
====
Use of this source code is governed by a BSD-style
license that can be found in the License file.

email
====
dongfuye at 163 dot com
