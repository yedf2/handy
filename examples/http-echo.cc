#include "http.h"
#include "logging.h"
#include "daemon.h"

using namespace std;
using namespace handy;

int main(int argc, const char* argv[]) {
    int threads = 1;
    if (argc > 1) {
        threads = atoi(argv[1]);
    }
    setloglevel("TRACE");
    MultiBase base(threads);
    HttpServer sample(&base, "", 8081);
    sample.onGet("/hello", [](const HttpConnPtr& con) {
        HttpResponse resp;
        resp.body = Slice("hello world");
        con->sendResponse(resp);
    });
    Signal::signal(SIGINT, [&]{base.exit();});
    base.loop();
    return 0;
}
