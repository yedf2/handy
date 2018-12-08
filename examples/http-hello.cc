#include <handy/handy.h>

using namespace std;
using namespace handy;

int main(int argc, const char *argv[]) {
    int threads = 1;
    if (argc > 1) {
        threads = atoi(argv[1]);
    }
    setloglevel("INFO");
    MultiBase base(threads);
    HttpServer sample(&base);
    sample.setConnCb([]() {
       TcpConnPtr conn = std::make_shared<TcpConn>();
       TcpConnPool::get_pool().register_state_cb(HTTP, conn);
        return conn;
    });
    int r = sample.bind("", 8081);
    exitif(r, "bind failed %d %s", errno, strerror(errno));
    sample.onGet("/hello", [](const HttpConnPtr &con) {
        string v = con.getRequest().version;
        HttpResponse resp;
        resp.body = Slice("hello world");
        con.sendResponse(resp);
        if (v == "HTTP/1.0") {
            con->close();
        }
    });
    Signal::signal(SIGINT, [&] { base.exit(); });
    base.loop();
    return 0;
}
