#include <handy/handy.h>

using namespace std;
using namespace handy;

int main(int argc, const char* argv[]) {
    setloglevel("TRACE");
    EventBase base;
    HSHAUPtr hsha = HSHAU::startServer(&base, "", 2099, 1);
    exitif(!hsha, "bind failed");
    Signal::signal(SIGINT, [&, hsha]{ base.exit(); hsha->exit(); signal(SIGINT, SIG_DFL);});

    hsha->onMsg([](const UdpServerPtr& con, const string& input, Ip4Addr addr){
        int ms = rand() % 1000 + 500;
        info("processing a msg: %.*s will using %d ms", (int)input.length(), input.data(), ms);
        usleep(ms * 1000);
        info("msg processed");
        return util::format("%s used %d ms", input.c_str(), ms);
    });
    for (int i = 0; i < 1; i ++) {
        UdpConnPtr con = UdpConn::createConnection(&base, "localhost", 2099);
        con->onMsg([](const UdpConnPtr& con, Buffer buf) {
            info("%.*s recved", (int)buf.size(), buf.data());
            con->close();
        });
        con->send("hello");
    }
    base.runAfter(500, [&,hsha]{info("exiting"); base.exit(); hsha->exit(); });
    base.loop();
    info("program exited");
}
