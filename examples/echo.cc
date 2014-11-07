#include "handy.h"
#include "logging.h"
#include "daemon.h"

using namespace std;
using namespace handy;


int main(int argc, const char* argv[]) {
    if (argc == 1 || strcmp(argv[1], "-h") == 0) {
        printf("usage %s <port> <threads>\n", argv[0]);
        return 1;
    }
    int port = 99;
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    int threads = 1;
    if (argc > 2) {
        threads = atoi(argv[2]);
    }
    if (argc > 3) {
        Logger::getLogger().setLogLevel(argv[2]);
    }

    MultiBase bases(threads);
    Signal::signal(SIGINT, [&]{ bases.exit(); });

    TcpServer echo(&bases, "", port);
    echo.onConnRead(
        [](const TcpConnPtr& con) { 
            con->send(con->getInput());
        }
    );
    bases.loop();
    info("program exited");
}