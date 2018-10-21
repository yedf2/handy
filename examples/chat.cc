#include <handy/handy.h>
#include <map>

using namespace std;
using namespace handy;

int main(int argc, const char* argv[]) {
    setloglevel("TRACE");
    map<intptr_t, TcpConnPtr> users; //the life cycle is longer than the connection and must be placed before the Base
    EventBase base;
    Signal::signal(SIGINT, [&]{ base.exit(); });

    int userid = 1;
    TcpServerPtr chat = TcpServer::startServer(&base, "", 99);
    exitif(chat == NULL, "start tcpserver failed");
    chat->onConnCreate([&]{
        TcpConnPtr con(new TcpConn);
        con->onState([&](const TcpConnPtr& con) {
            if (con->getState() == TcpConn::Connected) {
                con->context<int>() = userid;
                const char* welcome = "<id> <msg>: send msg to <id>\n<msg>: send msg to all\n\nhello %d";
                con->sendMsg(util::format(welcome, userid));
                users[userid] = con;
                userid ++;
            } else if (con->getState() == TcpConn::Closed) {
                users.erase(con->context<int>());
            }
        });
        con->onMsg(new LineCodec, [&](const TcpConnPtr& con, Slice msg){
            if (msg.size() == 0) { //ignore empty messages
                return;
            }
            int cid = con->context<int>();
            char* p = (char*)msg.data();
            intptr_t id = strtol(p, &p, 10);
            p += *p == ' '; //ignore a space
            string resp = util::format("%ld# %.*s", cid, msg.end()-p, p);

            int sended = 0;
            if (id == 0) { //send it to all other users
                for(auto& pc: users) {
                    if (pc.first != cid) {
                        sended ++;
                        pc.second->sendMsg(resp);
                    }
                }
            } else { //send to a specific user
                auto p1 = users.find(id);
                if (p1 != users.end()) {
                    sended ++;
                    p1->second->sendMsg(resp);
                }
            }
            con->sendMsg(util::format("#sended to %d users", sended));
        });
        return con;
    });
    base.loop();
    info("program exited");
    return 0;
}

