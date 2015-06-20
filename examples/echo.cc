#include <handy/handy.h>

using namespace std;
using namespace handy;


int main(int argc, const char* argv[]) {
    EventBase base;
    Signal::signal(SIGINT, [&]{ base.exit(); });
    TcpServer echo(&base);
    int r = echo.bind("", 99);
    exitif(r, "bind failed %d %s", errno, strerror(errno));
    echo.onConnRead([](const TcpConnPtr& con) {
        con->send(con->getInput());
    });
    base.loop();
}