#include <handy/handy.h>

using namespace std;
using namespace handy;

int main(int argc, const char* argv[]) {
    setloglevel("TRACE");
    EventBase base;
    HSHAPtr hsha = HSHA::startServer(&base, "", 2099, 4);
    exitif(!hsha, "bind failed");
    Signal::signal(SIGINT, [&, hsha]{ base.exit(); hsha->exit(); signal(SIGINT, SIG_DFL);});

    hsha->onMsg(new LineCodec, [](const TcpConnPtr& con, const string& input){
        int ms = rand() % 1000;
        info("processing a msg");
        usleep(ms * 1000);
        return util::format("%s used %d ms", input.c_str(), ms);
    });
    for (int i = 0; i < 5; i ++) {
        TcpConnPtr con = TcpConn::createConnection(&base, "localhost", 2099);
        con->onMsg(new LineCodec, [](const TcpConnPtr& con, Slice msg) {
            info("%.*s recved", (int)msg.size(), msg.data());
            con->close();
        });
        con->onState([](const TcpConnPtr& con) {
            if (con->getState() == TcpConn::Connected) {
                con->sendMsg("hello");
            }
        });
    }
    base.runAfter(1000, [&, hsha]{base.exit(); hsha->exit(); });
    base.loop();
    info("program exited");
}
