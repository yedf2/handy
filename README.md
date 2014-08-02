handy
====

a HANDY network C++11 libray on linux.

reactor 模式

支持优雅退出

代码简短

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
