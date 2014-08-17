#include "http.h"
#include "logging.h"
#include "daemon.h"

using namespace std;
using namespace handy;

int main() {
    Logger::getLogger().setLogLevel("WARN");
    EventBase base;
    HttpServer sample(&base, Ip4Addr(8081));
    sample.onGet("/hello", [](const HttpConn& con) {
        HttpResponse resp;
        resp.body = Slice("hello world");
        con.send(resp);
    });
    Signal::signal(SIGINT, [&]{base.exit();});
    base.loop();
    return 0;
}
