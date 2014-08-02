#include "handy.h"
#include "daemon.h"

using namespace std;
using namespace handy;


int main(int argc, const char* argv[]) {
    if (argc == 1 || strcmp(argv[1], "-h") == 0) {
        printf("usage %s <port>\n", argv[0]);
        return 1;
    }
    int port = 99;
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    if (argc > 2) {
        Logger::getLogger().setLogLevel(argv[2]);
    }

    EventBase base;
    Signal::signal(SIGINT, [&]{ base.exit(); });

    TcpServer echo(&base, Ip4Addr(port));
    echo.onConnRead(
        [](const TcpConnPtr& con) { 
            con->send(con->getInput());
        }
    );
    base.loop();
    info("program exited");
}