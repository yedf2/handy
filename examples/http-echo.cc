#include <handy/http.h>
#include <handy/daemon.h>

using namespace std;
using namespace handy;

int main(int argc, const char* argv[]) {
    int threads = 1;
    if (argc > 1) {
        threads = atoi(argv[1]);
    }
    setloglevel("TRACE");
    MultiBase base(threads);
    HttpServer sample(&base);
    int r = sample.bind("", 8081);
    exitif(r, "bind failed %d %s", errno, strerror(errno));
    sample.onGet("/hello", [](const HttpConnPtr& con) {
        HttpResponse resp;
        resp.body = Slice("hello world");
        con.sendResponse(resp);
    });
    Signal::signal(SIGINT, [&]{base.exit();});
    base.loop();
    return 0;
}
