#include "http.h"
#include "logging.h"
#include "daemon.h"

using namespace std;
using namespace handy;

int main() {
    Logger::getLogger().setLogLevel("DEBUG");
    EventBase base;
    HttpServer sample(&base, "", 80);
    sample.onGet("/", [](const HttpConnPtr& con) {
        HttpResponse resp;
        resp.body = Slice("hello world");
        con->sendResponse(resp);
    });
    Signal::signal(SIGINT, [&]{base.exit();});
    base.loop();
    return 0;
}
