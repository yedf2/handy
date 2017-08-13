#include <handy/handy.h>

using namespace std;
using namespace handy;

int main(int argc, const char* argv[]) {
    setloglevel("TRACE");
    EventBase base;
    Signal::signal(SIGINT, [&]{ base.exit(); });
    TcpConnPtr con = TcpConn::createConnection(&base, "127.0.0.1", 2099, 3000);
    con->setReconnectInterval(3000);
    con->onMsg(new LengthCodec, [](const TcpConnPtr& con, Slice msg) {
        info("recv msg: %.*s", (int)msg.size(), msg.data());
    });
    con->onState([=](const TcpConnPtr& con) {
        info("onState called state: %d", con->getState());
        if (con->getState() == TcpConn::Connected) {
            con->sendMsg("hello");
        }
    });
    base.loop();
    info("program exited");
}