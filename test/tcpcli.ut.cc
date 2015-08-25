#include <handy/conn.h>
#include <handy/logging.h>
#include "test_harness.h"

using namespace std;
using namespace handy;

TcpConnPtr connectto(EventBase* base, const char* host, short port) {
    TcpConnPtr con1 = TcpConn::createConnection(base, host, port);
    con1->onState(
        [=](const TcpConnPtr con) {
            if (con->getState() == TcpConn::Connected) {
                con->send("GET / HTTP/1.1\r\n\r\n");
            } else if (con->getState() == TcpConn::Closed) {
                info("connection to %s %d closed", host, port);
            } else if (con->getState() == TcpConn::Failed) {
                info("connect to %s %d failed", host, port);
            }
    }
    );
    con1->onRead( 
        [=](const TcpConnPtr con) {
            printf("%s %d response length is %lu bytes\n", host, port, con->getInput().size());
            con->getInput().clear();
    }
    );
    return con1;
}

TEST(test::TestBase, tcpcli) {
    EventBase base;
    TcpConnPtr baidu = connectto(&base, "www.baidu.com", 80);
    TcpConnPtr c = connectto(&base, "www.baidu.com", 81);
    TcpConnPtr local = connectto(&base, "localhost", 10000);
    for (int i = 0; i < 5; i ++) {
        base.loop_once(50);
    }
    //ASSERT_EQ(TcpConn::Connected, baidu->getState());
    ASSERT_EQ(TcpConn::Handshaking, c->getState());
    ASSERT_EQ(TcpConn::Failed, local->getState());
}
