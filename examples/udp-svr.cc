// echo server
#include <handy/handy.h>
using namespace handy;

int main(int argc, const char* argv[]) {
    setloglevel("TRACE");
    EventBase base;
    Signal::signal(SIGINT, [&]{ base.exit(); });
    UdpServerPtr svr = UdpServer::startServer(&base, "", 2099);
    exitif(!svr, "start udp server failed");
    svr->onMsg([](const UdpServerPtr& p, Buffer buf, Ip4Addr peer) {
        info("echo msg: %s to %s", buf.data(), peer.toString().c_str());
        p->sendTo(buf, peer);
    });
    base.loop();
}
