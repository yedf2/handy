#include <handy.h>
#include <logging.h>
#include <daemon.h>
#include "ssl-conn.h"

using namespace handy;
using namespace std;
int main(int argc, const char* argv[]) {
    Logger::getLogger().setLogLevel(Logger::LTRACE);
    EventBase ebase;
    Signal::signal(SIGINT, [&]{ebase.exit();});
    TcpConnPtr con;
    if (argc == 1) {
        info("connecting to www.openssl.org:443");
        con = TcpConn::createConnection<SSLConn>(&ebase, "www.openssl.org", 443, 0);
    } else if (argc == 2) {
        info("connecting to www.baidu.com:80");
        con = TcpConn::createConnection(&ebase, "www.baidu.com", 80, 0);
    } else if (argc == 3) {
        info("connecting to %s:%s", argv[1], argv[2]);
        con = TcpConn::createConnection<SSLConn>(&ebase, argv[1], atoi(argv[2]), 0);
    }
    fatalif(!con, "connectTo error");
    if (!con) {
        printf("connection error exit 1\n");
        return 1;
    }
    con->onState([&](const TcpConnPtr& con) {
        if (con->getState() == TcpConn::Connected) {
            info("sending request");
            con->send("GET / HTTP/1.1\r\n\r\n");
        } else {
            info("connection closed %d", con->getState());
            ebase.exit();
        }
    });
    con->onRead([](const TcpConnPtr& con) {
        Buffer& inbuf = con->getInput();
        info("%.*s\n", (int)inbuf.size(), inbuf.data());
        inbuf.clear();
    });
    ebase.loop();
    return 0;
}