#include <handy/handy.h>

using namespace std;
using namespace handy;

int main(int argc, const char* argv[]) {
    if (argc < 5) {
        printf("usage %s <host> <begin port> <end port> <conn count>\n", argv[0]);
        return 1;
    }
    string host = argv[1];
    int begin_port = atoi(argv[2]);
    int end_port = atoi(argv[3]);
    int conn_count = atoi(argv[4]);
    char buf[255] = "hello";
    EventBase base;
    vector<TcpConnPtr> allConns(conn_count);
    int send = 0;
    int connected = 0;
    int retry = 0;
    info("creating %d connections", conn_count);
    function<void(const TcpConnPtr& con, int i)> statecb = [&](const TcpConnPtr& con, int i) {
        TcpConn::State st = con->getState();
        if (st == TcpConn::Connected) {
            connected ++;
            if (connected % 10000 == 0) {
                info("%d connection connected", connected);
            }
            if (0 && connected == conn_count) {
                for(size_t i = 0; i < allConns.size(); i ++) {
                    if (allConns[i]->getState() == TcpConn::Connected) {
                        allConns[i]->send(buf, sizeof buf);
                        send++;
                    }
                }
                info("first run %d msgs sended", send);
            }
        } else if (st == TcpConn::Failed || st == TcpConn::Closed) {
            if (st == TcpConn::Closed) { connected --;}
            retry ++;
            if (errno != EINPROGRESS)
                info("error for %d conn %d %s", i, errno, strerror(errno));
            auto con = TcpConn::createConnection(&base, host, begin_port + (i % (end_port-begin_port)), 3000);
            con->onState([i, &statecb](const TcpConnPtr& con){statecb(con, i);});
            allConns[i] = con;
        }
    };
    for (int i = 0; i < conn_count; i ++) {
        auto con = TcpConn::createConnection(&base, host, begin_port + (i % (end_port-begin_port)), 3000);
        con->onRead([&send](const TcpConnPtr& con) { con->send(con->getInput()); send ++; });
        con->onState([i, &statecb](const TcpConnPtr& con){statecb(con, i);});
        allConns[i] = con;
        if ((i+1) % 10000 == 0) {
            info("%d connection created", i+1);
        }
    }
    int last_send = 0;
    base.runAfter(3000, [&] {
        info("%d qps %d msgs sended %d connected %d disconntected %d retry",
             (send - last_send) / 3, send, connected, conn_count - connected, retry);
        last_send = send;
    }, 3000);
    base.loop();
    info("program exited");
}
