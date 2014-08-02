#pragma  once
#include <string>
#include <stdio.h>

#ifdef NDEBUG
#define hlog(level, ...) \
    Logger::getLogger().logv(level, __FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define hlog(level, ...) snprintf(0, 0, __VA_ARGS__), \
    Logger::getLogger().logv(level, __FILE__, __LINE__, __func__, __VA_ARGS__)
#endif

#define debug(...) hlog(Logger::LDEBUG, __VA_ARGS__)
#define info(...) hlog(Logger::LINFO, __VA_ARGS__)
#define warn(...) hlog(Logger::LWARN, __VA_ARGS__)
#define error(...) hlog(Logger::LERROR, __VA_ARGS__)
#define fatal(...) hlog(Logger::LFATAL, __VA_ARGS__)
#define fatalif(b, ...) if((b)) hlog(Logger::LFATAL, __VA_ARGS__)

namespace handy {

struct Logger {
    enum LogLevel{LFATAL=0, LERROR, LUERR, LWARN, LINFO, LTRACE, LDEBUG, };
    Logger();
    ~Logger();
    void logv(int level, const char* file, int line, const char* func, const char* fmt ...);
    void setFileName(const char* filename);
    void setLogLevel(LogLevel level) { level_ = std::min(LDEBUG, std::max(LFATAL, level)); }
    void setLogLevel(const char* level);
    LogLevel getLogLevel() { return level_; }
    void setRotateInterval(long rotateInterval) { rotateInterval_ = rotateInterval; }
    static Logger& getLogger();
private:
    void maybeRotate();
    static const char* levelStrs_[LDEBUG+1];
    int fd_;
    LogLevel level_;
    long lastRotate_;
    long rotateInterval_;
    std::string filename_;
};

}

