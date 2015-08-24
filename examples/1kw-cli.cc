#include <handy/handy.h>

using namespace std;
using namespace handy;

int main(int argc, const char* argv[]) {
    if (argc < 5) {
        printf("usage %s <host> <begin port> <end port> <conn count> [hearbeat interval] [send size]\n", argv[0]);
        return 1;
    }
    string host = argv[1];
    int begin_port = atoi(argv[2]);
    int end_port = atoi(argv[3]);
    int conn_count = atoi(argv[4]);
    int heartbeat_interval = argc > 5 ? atoi(argv[5]) : 0;
    int bsz = argc > 6 ? atoi(argv[6]) : 0;
    
    char *buf = new char[bsz];
    char heartbeat[] = "heartbeat";
    int send = 0;
    int connected = 0;
    int retry = 0;
    int recved = 0;

    EventBase base;
    vector<TcpConnPtr> allConns(conn_count);
    info("creating %d connections", conn_count);
    for (int i = 0; i < conn_count; i ++) {
        auto con = TcpConn::createConnection(&base, host, begin_port + (i % (end_port-begin_port)), 3000);
        allConns[i] = con;
        con->setReconnectInterval(3000);
        con->onRead([&](const TcpConnPtr &con) {
            if (bsz && con->getInput().size() >= bsz) { //收到完整的回应后，再次发送数据
                con->getInput().clear();
                con->send(buf, bsz);
                send++;
                recved ++;
            } else if (heartbeat_interval && con->getInput().size() >= sizeof heartbeat) { //收到心跳包
                con->getInput().clear();
                recved ++;
            }
        });
        con->onState([&, i](const TcpConnPtr& con) {
            TcpConn::State st = con->getState();
            if (st == TcpConn::Connected) {
                connected ++;
                if (connected % 10000 == 0 || connected == conn_count) {
                    info("%d connection connected", connected);
                }
                if (bsz && connected == conn_count) { // 所有连接创建完成之后发送数据包
                    for(size_t i = 0; i < allConns.size(); i ++) {
                        if (allConns[i]->getState() == TcpConn::Connected) {
                            allConns[i]->send(buf, bsz);
                            send++;
                        }
                    }
                    info("first run %d msgs sended", send);
                }
            } else if (st == TcpConn::Failed || st == TcpConn::Closed) { //连接出错
                if (st == TcpConn::Closed) { connected --;}
                retry ++;
            }
        });
        if ((i+1) % 5000 == 0 || i+1 == conn_count) {
            info("%d connection created", i+1);
        }
    }
    if (heartbeat_interval) { //发送心跳
        base.runAfter(heartbeat_interval * 1000, [&] {
            for(size_t i = 0; i < allConns.size(); i ++) {
                if (allConns[i]->getState() == TcpConn::Connected) {
                    allConns[i]->send(heartbeat, sizeof heartbeat);
                    send++;
                }
            }
            info("send heartbeat");
        }, heartbeat_interval*1000);
    }

    //输出统计信息
    int last_send = 0;
    base.runAfter(3000, [&] {
        info("%d qps %d msgs sended %d connected %d disconntected %d retry %d recved",
             (send - last_send) / 3, send, connected, conn_count - connected, retry, recved);
        last_send = send;
    }, 3000);
    base.loop();
    delete buf;
    info("program exited");
}
