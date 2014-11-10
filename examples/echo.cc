#include "conn.h"
#include "logging.h"
#include "daemon.h"

using namespace std;
using namespace handy;


int main(int argc, const char* argv[]) {
    setloglevel("TRACE");
    EventBase bases;
    Signal::signal(SIGINT, [&]{ bases.exit(); });
    TcpServer echo(&bases, "", 99);
    echo.onConnRead([](const TcpConnPtr& con) {
        con->send(con->getInput());
    });
    bases.loop();
    info("program exited");
}