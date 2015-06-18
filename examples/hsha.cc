#include <handy/conn.h>
#include <handy/daemon.h>

using namespace std;
using namespace handy;


int main(int argc, const char* argv[]) {
    setloglevel("TRACE");
    EventBase base;
    ThreadPool workers(4);
    Signal::signal(SIGINT, [&]{ base.exit(); workers.exit(); signal(SIGINT, SIG_DFL);});

    TcpServer hsha(&base);
    int r = hsha.bind("", 99);
    exitif(r, "bind failed %d %s", errno, strerror(errno));
    hsha.onConnCreate([&]{
        TcpConnPtr con(new TcpConn);
        con->setCodec(new LineCodec);
        con->onMsg([&] (const TcpConnPtr& con, Slice msg) {
            string s(msg);
            workers.addTask([s, con, &base] {
                int ms = rand() % 1000;
                usleep(ms * 1000);
                base.safeCall([s, con, ms] {
                    con->send(util::format("%s used %d ms", s.c_str(), ms));
                });
            });
        });
        return con;
    });
    base.loop();
    workers.join();
    info("program exited");
}
