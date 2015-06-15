#include "daemon.h"
#include <functional>
#include <utility>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <map>
#include <string>

using namespace std;

namespace handy {

namespace {

struct ExitCaller {
    ~ExitCaller() { functor_(); }
    ExitCaller(std::function<void()>&& functor): functor_(std::move(functor)) {}
private:
    std::function<void()> functor_;
};

}

static int writePidFile(const char *pidfile)
{
    char str[32];
    int lfp = open(pidfile, O_WRONLY|O_CREAT|O_TRUNC, 0600);
    if (lfp < 0 || lockf(lfp, F_TLOCK, 0) < 0) {
        fprintf(stderr, "Can't write Pid File: %s", pidfile);
        return -1;
    }
    ExitCaller call1([=] { close(lfp); });
    sprintf(str, "%d\n", getpid());
    ssize_t len = strlen(str);
    ssize_t ret = write(lfp, str, len);
    if (ret != len ) {
        fprintf(stderr, "Can't Write Pid File: %s", pidfile);
        return -1;
    }
    return 0;
}

int Daemon::getPidFromFile(const char *pidfile)
{
    char buffer[64], *p;
    int lfp = open(pidfile, O_RDONLY, 0);
    if (lfp < 0) {
        return lfp;
    }
    ssize_t rd = read(lfp, buffer, 64);
    close(lfp);
    if (rd <= 0) {
        return -1;
    }
    buffer[63] = '\0';
    p = strchr(buffer, '\n');
    if (p != NULL)
        *p = '\0';
    return atoi(buffer);
}


int Daemon::daemonStart(const char* pidfile) {
    int pid = getPidFromFile(pidfile);
    if (pid > 0) {
        if (kill(pid, 0) == 0 || errno == EPERM) {
            fprintf(stderr, "daemon exists, use restart\n");
            return -1;
        }
    }
    if (getppid() == 1) {
        fprintf(stderr, "already daemon, can't start\n");
        return -1;
    }

    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "fork error: %d\n", pid);
        return -1;
    }
    if (pid > 0) {
        exit(0); // parent exit
    }
    setsid();
    int r = writePidFile(pidfile);
    if (r != 0) {
        return r;
    }
    int fd =open("/dev/null", 0);
    if (fd >= 0) {
        close(0);
        dup2(fd, 0);
        dup2(fd, 1);
        close(fd);
        string pfile = pidfile;
        static ExitCaller del([=] {
            unlink(pfile.c_str());
        });
        return 0;
    }
    return -1;
}

int Daemon::daemonStop(const char* pidfile) {
    int pid = getPidFromFile(pidfile);
    if (pid <= 0) {
        fprintf(stderr, "%s not exists or not valid\n", pidfile);
        return -1;
    }
    int r = kill(pid, SIGQUIT);
    if (r < 0) {
        fprintf(stderr, "program %d not exists\n", pid);
        return r;
    }
    for (int i = 0; i < 300; i++) {
        usleep(10*1000);
        r = kill(pid, SIGQUIT);
        if (r != 0) {
            fprintf(stderr, "program %d exited\n", pid);
            unlink(pidfile);
            return 0;
        }
    }
    fprintf(stderr, "signal sended to process, but still exists after 3 seconds\n");
    return -1;
}

int Daemon::daemonRestart(const char* pidfile) {
    int pid = getPidFromFile(pidfile);
    if (pid > 0) {
        if (kill(pid, 0) == 0) {
            int r = daemonStop(pidfile);
            if (r < 0) {
                return r;
            }
        } else if (errno == EPERM) {
            fprintf(stderr, "do not have permission to kill process: %d\n", pid);
            return -1;
        }
    } else {
        fprintf(stderr, "pid file not valid, just ignore\n");
    }
    return daemonStart(pidfile);
}

void Daemon::daemonProcess(const char* cmd, const char* pidfile) {
    int r = 0;
    if (cmd == NULL || strcmp(cmd, "start")==0) {
        r = daemonStart(pidfile);
    } else if (strcmp(cmd, "stop")==0) {
        r = daemonStop(pidfile);
        if (r == 0) {
            exit(0);
        }
    } else if (strcmp(cmd, "restart")==0) {
        r = daemonRestart(pidfile);
    } else {
        fprintf(stderr, "ERROR: bad daemon command. exit\n");
        r = -1;
    }
    if (r) {
        //exit on error
        exit(1);
    }
}

void Daemon::changeTo(const char* argv[]) {
    int pid = getpid();
    int r = fork();
    if (r < 0) {
        fprintf(stderr, "fork error %d %s", errno, strerror(errno));
    } else if (r > 0) { //parent;
        return;
    } else { //child
        //wait parent to exit
        while(kill(pid, 0) == 0) {
            usleep(10*1000);
        }
        if (errno != ESRCH) {
            const char* msg = "kill error\n";
            ssize_t w1 = write(2, msg, strlen(msg));
            (void)w1;
            _exit(1);
        }
        execvp(argv[0], (char* const*)argv);
    }
}

namespace {
    map<int, function<void()>> handlers;
    void signal_handler(int sig) {
        handlers[sig]();
    }
}

void Signal::signal(int sig, const function<void()>& handler) {
    handlers[sig] = handler;
    ::signal(sig, signal_handler);
}

}