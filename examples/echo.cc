#include <handy/conn.h>
#include <handy/daemon.h>

using namespace std;
using namespace handy;


int main(int argc, const char* argv[]) {
    EventBase bases;
    Signal::signal(SIGINT, [&]{ bases.exit(); });
    TcpServer echo(&bases);
    int r = echo.bind("", 99);
    exitif(r, "bind failed %d %s", errno, strerror(errno));
    echo.onConnRead([](const TcpConnPtr& con) {
        con->send(con->getInput());
    });
    bases.loop();
}