#include <handy/handy.h>

using namespace std;
using namespace handy;

int main(int argc, const char* argv[]) {
    if (argc <= 2) {
        printf("usage: %s <begin port> <end port>\n", argv[0]);
        return 1;
    }
    int begin_port = atoi(argv[1]);
    int end_port = atoi(argv[2]);
    EventBase base;
    vector<TcpServerPtr> svrs(end_port - begin_port);
    for (int i = 0; i < end_port-begin_port; i ++) {
        svrs[i] = TcpServer::startServer(&base, "", begin_port + i);
        svrs[i]->onConnRead([](const TcpConnPtr& con) { con->send(con->getInput()); });
    }
    base.loop();
    info("program exited");
}
