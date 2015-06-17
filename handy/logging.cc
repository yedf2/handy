#include "logging.h"
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <time.h>
#include <utility>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include "port_posix.h"

using namespace std;

namespace handy {


Logger::Logger(): level_(LINFO), lastRotate_(time(NULL)), rotateInterval_(86400) {
    tzset();
    fd_ = -1;
    realRotate_ = lastRotate_;
}

Logger::~Logger() {
    if (fd_ != -1) {
        close(fd_);
    }
}

const char* Logger::levelStrs_[LALL+1] = {
    "FATAL",
    "ERROR",
    "UERR",
    "WARN",
    "INFO",
    "DEBUG",
    "TRACE",
    "ALL",
};

Logger& Logger::getLogger() {
    static Logger logger;
    return logger;
}

void Logger::setLogLevel(const string& level) {
    LogLevel ilevel = LINFO;
    for (size_t i = 0; i < sizeof(levelStrs_)/sizeof(const char*); i++) {
        if (strcasecmp(levelStrs_[i], level.c_str()) == 0) {
            ilevel = (LogLevel)i;
            break;
        }
    }
    setLogLevel(ilevel);
}

void Logger::setFileName(const string& filename) {
    int fd = open(filename.c_str(), O_APPEND|O_CREAT|O_WRONLY|O_CLOEXEC, DEFFILEMODE);
    if (fd < 0) {
        fprintf(stderr, "open log file %s failed. msg: %s ignored\n",
                filename.c_str(), strerror(errno));
        return;
    }
    filename_ = filename;
    if (fd_ == -1) {
        fd_ = fd;
    } else {
        int r = dup2(fd, fd_);
        fatalif(r<0, "dup2 failed");
        close(fd);
    }
}

void Logger::maybeRotate() {
    time_t now = time(NULL);
    if (filename_.empty() || (now - timezone) / rotateInterval_ == (lastRotate_ - timezone)/ rotateInterval_) {
        return;
    }
    lastRotate_ = now;
    long old = realRotate_.exchange(now);
    //如果realRotate的值是新的，那么返回，否则，获得了旧值，进行rotate
    if ((old - timezone) / rotateInterval_ == (lastRotate_ - timezone) / rotateInterval_) {
        return;
    }
    struct tm ntm;
    localtime_r(&now, &ntm);
    char newname[4096];
    snprintf(newname, sizeof(newname), "%s.%d%02d%02d%02d%02d",
        filename_.c_str(), ntm.tm_year + 1900, ntm.tm_mon + 1, ntm.tm_mday,
        ntm.tm_hour, ntm.tm_min);
    const char* oldname = filename_.c_str();
    int err = rename(oldname, newname);
    if (err != 0) {
        fprintf(stderr, "rename logfile %s -> %s failed msg: %s\n",
            oldname, newname, strerror(errno));
        return;
    }
    int fd = open(filename_.c_str(), O_APPEND | O_CREAT | O_WRONLY | O_CLOEXEC, DEFFILEMODE);
    if (fd < 0) {
        fprintf(stderr, "open log file %s failed. msg: %s ignored\n",
            newname, strerror(errno));
        return;
    }
    dup2(fd, fd_);
    close(fd);
}

static thread_local uint64_t tid;
void Logger::logv(int level, const char* file, int line, const char* func, const char* fmt ...) {
    if (tid == 0) {
        tid = port::gettid();
    }
    if (level > level_) {
        return;
    }
    maybeRotate();
    char buffer[4*1024];
    char* p = buffer;
    char* limit = buffer + sizeof(buffer);

    struct timeval now_tv;
    gettimeofday(&now_tv, NULL);
    const time_t seconds = now_tv.tv_sec;
    struct tm t;
    localtime_r(&seconds, &t);
    p += snprintf(p, limit - p,
        "%04d/%02d/%02d-%02d:%02d:%02d.%06d %lx %s %s:%d ",
        t.tm_year + 1900,
        t.tm_mon + 1,
        t.tm_mday,
        t.tm_hour,
        t.tm_min,
        t.tm_sec,
        static_cast<int>(now_tv.tv_usec),
        (long)tid,
        levelStrs_[level],
        file,
        line);
    va_list args;
    va_start(args, fmt);
    p += vsnprintf(p, limit-p, fmt, args);
    va_end(args);
    p = std::min(p, limit - 2);
    //trim the ending \n
    while (*--p == '\n') {
    }
    *++p = '\n';
    *++p = '\0';
    int fd = fd_ == -1 ? 1 : fd_;
    int err = ::write(fd, buffer, p - buffer);
    if (err != p-buffer) {
        fprintf(stderr, "write log file %s failed. written %d errmsg: %s\n",
            filename_.c_str(), err, strerror(errno));
    }
    if (level <= LERROR) {
        syslog(LOG_ERR, "%s", buffer+27);
    }
    if (level == LFATAL) {
        fprintf(stderr, "%s", buffer);
        assert(0);
    }
}

}