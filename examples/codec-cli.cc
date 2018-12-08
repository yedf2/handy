#include <handy/handy.h>

using namespace std;
using namespace handy;

int main(int argc, const char *argv[]) {
    setloglevel("TRACE");
    EventBase base;
    Signal::signal(SIGINT, [&] { base.exit(); });
    TcpConnPtr con = TcpConn::createConnection(&base, "127.0.0.1", 2098, 3000);
    con->setReconnectInterval(-1);
    con->onMsg(new OnlyLengthCodec, [&](const TcpConnPtr &con, Slice msg) {
        info("recv msg: %.*s", (int) msg.size(), msg.data());
//        con->close();
//        base.exit();

    });
    con->onState([&](const TcpConnPtr &con) {
        info("onState called state: %d", con->getState());
        if (con->getState() == TcpConn::Connected) {
            con->sendMsg("hello");
//            base.runAfter(1000, [&]()->void {
//                con->sendMsg("hello");
//            }, 1000);
        }
    });
    base.loop();
    info("program exited");
}