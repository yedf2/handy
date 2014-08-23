#include "http.h"
#include "logging.h"
#include "daemon.h"

using namespace std;
using namespace handy;

int main() {
    Logger::getLogger().setLogLevel("DEBUG");
    EventBase base;
    HttpServer sample(&base, Ip4Addr(80));
    sample.onGet("/", [](HttpConn* con) {
        HttpResponse resp;
        resp.body = Slice("hello world");
        con->sendResponse(resp);
    });
    Signal::signal(SIGINT, [&]{base.exit();});
    base.loop();
    return 0;
}
