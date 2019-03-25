#include <handy/handy.h>

using namespace std;
using namespace handy;



int main(int argc, const char *argv[]) {
    Logger::getLogger().setLogLevel(Logger::LINFO);
//    EventBase base;
    Signal::signal(SIGINT, [&] { EventBase::instance()->exit(); });

    TcpServerPtr echo = TcpServer::startServer(EventBase::instance(), "", 9999);
    exitif(echo == NULL, "start tcp server failed");
    
    echo->onConnCreate([] {
        TcpConnPtr con(new TcpConn);
        con->onMsg(new OnlyLengthCodec, [](const TcpConnPtr &con, Slice msg) {
            info("recv msg: %.*s", (int) msg.size(), msg.data());
            con->sendMsg(msg);
            TcpConnPool::get_pool().get_con(con->getChannel()->id())->sendMsg("test_pool");
        });

        TcpConnPool::get_pool().register_state_cb(con);

        return con;
    });
    EventBase::instance()->loop();
    info("program exited");
}