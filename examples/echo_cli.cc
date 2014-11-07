#include "handy.h"
#include "logging.h"
#include "daemon.h"
#include <signal.h>
#include <atomic>

using namespace std;
using namespace handy;


int main(int argc, const char* argv[]) {
    size_t port=99, conns=1, msgSize=10, sendCount=1;
    const char* host = "localhost";
    if (argc > 1 && argc < 6) {
        printf("usage %s <host> <port> <connections> <msg size> <msg count per connection>\n", argv[0]);
        return 1;
    }
    if (argc == 6) {
        int i = 0;
        host = argv[++i];
        port = atoi(argv[++i]);
        conns = atoi(argv[++i]);
        msgSize = atoi(argv[++i]);
        sendCount = atoi(argv[++i]);
    }
    atomic<int64_t> sended(0);
    int64_t lastsended = 0;
    string msg(msgSize, 'a');
    EventBase base;
    for (size_t i = 0; i < conns; i++) {
        TcpConnPtr con = TcpConn::createConnection(&base, host, port);
        con->context<ssize_t>() = sendCount;
        con->onRead([&](const TcpConnPtr& con) {
            if (con->getInput().size() >= msgSize) {
                con->send(con->getInput());
                ssize_t& cn = con->context<ssize_t>();
                if (--cn % 100 == 0) {
                    sended += 1000;
                }
                if (cn == 0) {
                    con->close();
                }
            }
        });
        con->onState([&](const TcpConnPtr& con) {
            if (con->getState() == TcpConn::Connected) {
                con->send(msg.data(), msg.size());
            }
        });
    }
    base.runAfter(3000, [&]{
        int64_t cn = sended.load();
        int64_t qps = (cn-lastsended)/3;
        int64_t mbytes = qps*msg.size() / 1000 /1000;
        info("cn %ld speed qps %ld throughput %ld M", cn, qps, mbytes);
        lastsended = sended;
    }, 3000);
    base.loop();
    info("program exited");
}