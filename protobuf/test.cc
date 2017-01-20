#include "proto_msg.h"
#include "msg.pb.h"

using namespace std;
using namespace handy;

void handleQuery(TcpConnPtr con, Query* query) {
    info("query recved name %s id %d", query->name().c_str(), query->id());
    delete query;
    con->getBase()->exit();
}

void testencode() {
    Query q;
    q.set_name("hello");
    q.set_id(123);
    Buffer b;
    ProtoMsgCodec dis;
    dis.encode(&q, b);
    Query* p = dynamic_cast<Query*> (dis.decode(b));
    info("p name %s id %d", p->name().c_str(), p->id());
    delete p;
}
int main() {
    Logger::getLogger().setLogLevel(Logger::LDEBUG);
    testencode();

    EventBase base;
    TcpServer echo(&base);
    int r = echo.bind("", 2099);
    exitif(r, "bind failed %d %s", errno, strerror(errno));
    ProtoMsgDispatcher dispatch;
    echo.onConnRead(
        [&](TcpConnPtr con) {
            if (ProtoMsgCodec::msgComplete(con->getInput())) {
                Message* msg = ProtoMsgCodec::decode(con->getInput());
                if (msg) {
                    dispatch.handle(con, msg);
                } else {
                    error("bad msg from connection data");
                    con->close();
                }
            }
        }
    );

    dispatch.onMsg<Query>(handleQuery);
    TcpConnPtr cmd = TcpConn::createConnection(&base, "localhost", 2099);
    cmd->onState([](const TcpConnPtr& con) { 
        if (con->getState() == TcpConn::Connected) {
            Query query;
            query.set_name("hello", 5);
            query.set_id(123);
            ProtoMsgCodec::encode(&query, con->getOutput());
            con->sendOutput();
        }
    });
    base.loop();
}
