#include "http.h"
#include "logging.h"
#include "daemon.h"
#include "stat-server.h"

using namespace std;
using namespace handy;

int main(int argc, const char* argv[]) {
    Logger::getLogger().setLogLevel("DEBUG");
    EventBase base;
    StatServer sample(&base, Ip4Addr(80));
    sample.onState("loglevel", "log level for server", 
        [](const HttpRequest& req, HttpResponse& resp){
            resp.body = Logger::getLogger().getLogLevelStr();
        }
    );
    sample.onState("pid", "process id of server", 
        [](const HttpRequest& req, HttpResponse& resp){
            resp.body = util::format("%d", getpid()); 
        }
    );
    sample.onCmd("lesslog", "set log to less detail", 
        [](const HttpRequest& req, HttpResponse& resp){ 
            Logger::getLogger().adjustLogLevel(-1);
            resp.body = "OK"; 
        }
    );
    sample.onCmd("morelog", "set log to more detail", 
        [](const HttpRequest& req, HttpResponse& resp){ 
            Logger::getLogger().adjustLogLevel(1);
            resp.body = "OK"; 
        }
    );
    sample.onCmd("restart", "restart program", 
        [&](const HttpRequest& req, HttpResponse& resp){ 
            resp.body = "restarting"; 
            base.runAfter(0, [&]{ base.exit(); Daemon::changeTo(argv);});
    }
    );
    sample.onCmd("stop", "stop program", 
        [&](const HttpRequest& req, HttpResponse& resp){ 
            resp.body = "restarting"; 
            base.runAfter(0, [&]{ base.exit();});
    }
    );
    sample.onPage("page", "show page content", 
        [](const HttpRequest& req, HttpResponse& resp){ 
            resp.body = "this is a page"; 
        }
    );
    Signal::signal(SIGINT, [&]{base.exit();});
    base.loop();
    return 0;
}
