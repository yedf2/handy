#include "handy.h"
#include "logging.h"
#include "daemon.h"
#include <signal.h>
#include <map>

using namespace std;
using namespace handy;


int main(int argc, const char* argv[]) {
    if (argc == 1 || strcmp(argv[1], "-h") == 0) {
        printf("usage %s <port>\n", argv[0]);
        return 1;
    }
    int port = 99;
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    if (argc > 2) {
        Logger::getLogger().setLogLevel(argv[2]);
    }

    intptr_t userid = 1;
    map<intptr_t, TcpConnPtr> users;
    string hlp = "<id> <msg>: send msg to <id>\n<msg>: send msg to all\n";
    EventBase base;
    Signal::signal(SIGINT, [&]{ base.exit(); });

    TcpServer chat(&base, Ip4Addr(port));
    chat.onConnState([&](const TcpConnPtr& con) {
        if (con->getState() == TcpConn::Connected) {
            con->setContext((void*)userid);
            con->send(util::format("hello %d\n", userid));
            con->send(hlp);
            users[userid] = con;
            userid ++;
        } else if (con->getState() == TcpConn::Closed) {
            intptr_t id = (intptr_t)con->getContext();
            users.erase(id);
        }
    });
    chat.onConnRead(
        [&](const TcpConnPtr& con) {
            intptr_t cid = (intptr_t)con->getContext();
            Buffer& buf = con->getInput();
            const char* ln = NULL;
            //one line per iteration
            while (buf.size() && (ln = strchr(buf.data(), '\n')) != NULL) {
                char* p = buf.data();
                if (ln == p) { //empty line
                    buf.consume(1);
                    continue;
                }
                intptr_t id = strtol(p, &p, 10);
                auto p1 = users.find(id);
                if (p1 == users.end()) { //to all user
                    for(auto& pc: users) {
                        pc.second->send(util::format("%ld# %.*s", cid, ln+1-p, p));
                    }
                } else { //to one user
                    p1->second->send(util::format("%ld#", (intptr_t)con->getContext()));
                    p1->second->send(string(p, ln+1-p));
                }
                buf.consume(ln - buf.data()+1);
            }
        }
    );
    base.loop();
    info("program exited");
}