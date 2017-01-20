// echo client
#include <handy/handy.h>
using namespace handy;

int main(int argc, const char* argv[]) {
    setloglevel("TRACE");
    EventBase base;
    Signal::signal(SIGINT, [&]{ base.exit(); });
    UdpConnPtr con = UdpConn::createConnection(&base, "127.0.0.1", 2099);
    exitif(!con, "start udp conn failed");
    con->onMsg([](const UdpConnPtr& p, Buffer buf) {
        info("udp recved %lu bytes", buf.size());
    });
    base.runAfter(0, [=](){con->send("hello");}, 1000);
    base.loop();
}

