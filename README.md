handy
====

a HANDY network C++11 libray on linux.

reactor 模式

支持优雅退出

无锁日志系统，按时间间隔轮替

代码简短

ubuntu14 64位/g++ 4.8.1上通过测试

安装
====
./build_detect_platform && make

examples
====
<pre><code>
#include "handy.h"
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
