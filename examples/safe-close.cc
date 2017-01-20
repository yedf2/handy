#include <handy/handy.h>
using namespace handy;

int main(int argc, const char* argv[]) {
    EventBase base;
    Signal::signal(SIGINT, [&]{ base.exit(); });
    TcpServerPtr svr = TcpServer::startServer(&base, "", 2099);
    exitif(svr == NULL, "start tcp server failed");
    TcpConnPtr con = TcpConn::createConnection(&base, "localhost", 2099);
    std::thread th([con,&base](){
        sleep(1);
        info("thread want to close an connection");
        base.safeCall([con](){ con->close(); }); //其他线程需要操作连接，应当通过safeCall把操作交给io线程来做
    });
    base.runAfter(1500, [&base](){base.exit();});
    base.loop();
    th.join();
}