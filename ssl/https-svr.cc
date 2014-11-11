#include <handy.h>
#include <logging.h>
#include <daemon.h>
#include "ssl-conn.h"
#include <http.h>

using namespace handy;
using namespace std;

int main(int argc, const char* argv[]) {
    Logger::getLogger().setLogLevel(Logger::LTRACE);
    EventBase ebase;
    Signal::signal(SIGINT, [&]{ ebase.exit(); });
    SSLConn::setSSLCertKey("server.pem", "server.pem");
    HttpServer ss(&ebase, "0.0.0.0", 1002);
    ss.setConnType<SSLConn>();
    ss.onGet("/hello", [](const HttpConnPtr& con) {
        HttpResponse resp;
        resp.body = Slice("hello world");
        con.sendResponse(resp);
    });
    ebase.loop();
    return 0;
}
