#include "handy.h"
#include "logging.h"
#include "daemon.h"

using namespace std;
using namespace handy;


int main(int argc, const char* argv[]) {
    Logger::getLogger().setLogLevel(Logger::LTRACE);
    EventBase base;
    Signal::signal(SIGINT, [&]{ base.exit(); });

    TcpServer echo(&base, "", 99);
    echo.onConnCreate([]{
        TcpConnPtr con(new TcpConn);
        con->setCodec(new LengthCodec);
        con->onMsg([](const TcpConnPtr& con, Slice msg) {
            info("recv msg: %.*s", (int)msg.size(), msg.data());
            con->sendMsg(msg);
        });
        return con;
    });
    base.loop();
    info("program exited");
}