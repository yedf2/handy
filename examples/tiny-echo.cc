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
