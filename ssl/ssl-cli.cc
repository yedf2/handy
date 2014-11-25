#include <handy.h>
#include <logging.h>
#include <daemon.h>
#include "ssl-conn.h"
#include <http.h>

using namespace handy;
using namespace std;

int main(int argc, const char* argv[]) {
    setloglevel("TRACE");
    EventBase ebase;
    Signal::signal(SIGINT, [&]{ ebase.exit(); });
    int finished = 0;
    TcpConnPtr con = TcpConn::createConnection<SSLConn>(&ebase, "www.openssl.com", 443);
    con->onState([&](const TcpConnPtr& con) {
        if (con->getState() == TcpConn::Connected) {
            con->send("GET / HTTP/1.1\r\nConnection: Close\r\n\r\n");
        } else {
            finished ++;
        }
    });
    con->onRead([](const TcpConnPtr& con) {
        Buffer& buf = con->getInput();
        int len = (int)buf.size();
        info("response %d bytes\n%.*s",len, len, buf.data());
        buf.clear();
    });
    HttpConnPtr hcon = TcpConn::createConnection<SSLConn>(&ebase, "www.openssl.com", 443);
    hcon->onState([&](const TcpConnPtr& con) {
        if (hcon->getState() == TcpConn::Connected) {
            hcon->send("GET / HTTP/1.1\r\nConnection: Close\r\n\r\n");
        } else {
            finished ++;
        }
    });
    hcon.onHttpMsg([](const HttpConnPtr& hcon) {
        Slice body = hcon.getResponse().getBody();
        info("response is: %.*s", (int)body.size(), body.data());
        hcon.clearData();
    });
    while (finished != 2 && !ebase.exited()) {
        ebase.loop_once(100);
    }
    return 0;
}
