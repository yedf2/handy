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
    int disconnected = 0;
    info("creating %d connections", conn_count);
    for (int i = 0; i < conn_count; i ++) {
        allConns[i] = TcpConn::createConnection(&base, host, begin_port + (i % (end_port-begin_port)));
        allConns[i]->onRead([&send](const TcpConnPtr& con) { con->send(con->getInput()); send ++; });
        allConns[i]->onState([&](const TcpConnPtr& con) {
            TcpConn::State st = con->getState();
            if (st == TcpConn::Connected) {
                connected ++;
                if (connected % 10000 == 0) {
                    info("%d connection connected", connected);
                }
                if (connected == conn_count) {
                    for(size_t i = 0; i < allConns.size(); i ++) {
                        if (allConns[i]->getState() == TcpConn::Connected) {
                            allConns[i]->send(buf, sizeof buf);
                            send++;
                        }
                    }
                    info("first run %d msgs sended", send);
                }
            } else if (st == TcpConn::Failed || st == TcpConn::Closed) {
                disconnected ++;
            }
        });
        if ((i+1) % 10000 == 0) {
            info("%d connection created", i+1);
        }
    }
    int last_send = 0;
    base.runAfter(3000, [&] {
        info("%d qps %d msgs sended %d connected %d disconntected", (send - last_send) / 3, send, connected, disconnected);
        last_send = send;
    }, 3000);
    base.loop();
    info("program exited");
}
