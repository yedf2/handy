#pragma once
#include <functional>
#include <signal.h>
namespace handy {

struct Daemon {
    //exit in parent
    static int daemonStart(const char* pidfile);
    //exit in parent
    static int daemonRestart(const char* pidfile);
    static int daemonStop(const char* pidfile);
    static int getPidFromFile(const char* pidfile);

    // cmd: start stop restart
    // exit(1) when error
    // exit(0) when start or restart in parent
    // return when start or restart in child
    static void daemonProcess(const char* cmd, const char* pidfile);
    //fork; wait for parent to exit; exec argv
    //you may use it to implement restart in program
    static void changeTo(const char* argv[]);
};

struct Signal {
    static void signal(int sig, const std::function<void()>& handler);
};
}
