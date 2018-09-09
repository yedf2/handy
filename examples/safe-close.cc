#include <handy/handy.h>
using namespace handy;

int main(int argc, const char *argv[]) {
    EventBase base;
    Signal::signal(SIGINT, [&] { base.exit(); });
    TcpServerPtr svr = TcpServer::startServer(&base, "", 2099);
    exitif(svr == NULL, "start tcp server failed");
    TcpConnPtr con = TcpConn::createConnection(&base, "localhost", 2099);
    std::thread th([con, &base]() {
        sleep(1);
        info("thread want to close an connection");
        base.safeCall([con]() { con->close(); });  //Other threads need to operate the connection, should be handed over to the io thread through safeCall
    });
    base.runAfter(1500, [&base]() { base.exit(); });
    base.loop();
    th.join();
}