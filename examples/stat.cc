#include <handy/handy.h>
#include <handy/stat-svr.h>

using namespace std;
using namespace handy;

int main(int argc, const char* argv[]) {
    Logger::getLogger().setLogLevel("DEBUG");
    EventBase base;
    StatServer sample(&base);
    int r = sample.bind("", 80);
    exitif(r, "bind failed %d %s", errno, strerror(errno));
    sample.onState("loglevel", "log level for server", []{return Logger::getLogger().getLogLevelStr(); });
    sample.onState("pid", "process id of server", [] { return util::format("%d", getpid()); });
    sample.onCmd("lesslog", "set log to less detail", []{ Logger::getLogger().adjustLogLevel(-1); return "OK"; });
    sample.onCmd("morelog", "set log to more detail", [] { Logger::getLogger().adjustLogLevel(1); return "OK"; });
    sample.onCmd("restart", "restart program", [&] { 
        base.safeCall([&]{ base.exit(); Daemon::changeTo(argv);}); 
        return "restarting"; 
    });
    sample.onCmd("stop", "stop program", [&] { base.safeCall([&]{base.exit();}); return "stoping"; });
    sample.onPage("page", "show page content", [] { return "this is a page"; });
    Signal::signal(SIGINT, [&]{base.exit();});
    base.loop();
    return 0;
}
