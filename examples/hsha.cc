#include "handy.h"
#include "logging.h"
#include "daemon.h"
#include <signal.h>

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

    EventBase base;
    ThreadPool workers(4);
    Signal::signal(SIGINT, [&]{ base.exit(); workers.exit(); signal(SIGINT, SIG_DFL);});

    TcpServer hsha(&base, "", port);
    hsha.onConnRead( [&workers, &base](const TcpConnPtr& con) {
        Buffer& b = con->getInput();
        while (b.size()) {
            const char* ln = strchr(b.data(), '\n');
            if (ln && ln != b.data()) { // a non-empty line
                string s((const char*)b.data(), ln);
                b.consume(s.size() + 1);
                workers.addTask([s, con, &base]{
                    int ms = rand() % 1000;
                    usleep(ms * 1000);
                    base.safeCall([s, con, ms] {
                        con->send(util::format("%s used %d ms\n", s.c_str(), ms));
                    });
                });
            } else if (ln) {
                b.consume(1);
            } else {
                break;
            }
        }
    });
    base.loop();
    workers.join();
    info("program exited");
}