#include <handy/handy.h>
using namespace handy;

int main(int argc, const char* argv[]) {
    setloglevel("TRACE");
    EventBase base;
    Signal::signal(SIGINT, [&]{ base.exit(); });
    TcpServerPtr svr = TcpServer::startServer(&base, "", 2099);
    exitif(svr == NULL, "start tcp server failed");
    svr->onConnState([&](const TcpConnPtr& con) { //200ms后关闭连接
        if (con->getState() == TcpConn::Connected)
            base.runAfter(200, [con](){ info("close con after 200ms"); con->close(); });
    });
    TcpConnPtr con1 = TcpConn::createConnection(&base, "localhost", 2099);
    con1->setReconnectInterval(300);
//    TcpConnPtr con2 = TcpConn::createConnection(&base, "localhost", 1, 100);
//    con2->setReconnectInterval(200);
    base.runAfter(600, [&](){base.exit();});
    base.loop();
}