#pragma once
#include <string>
#include <functional>
#include <utility>
#include <vector>
#include <string.h>
#include <stdlib.h>

namespace handy {

struct noncopyable {
    noncopyable() {};
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;
};

struct util {
    static std::string format(const char* fmt, ...);
    static int64_t timeMicro();
    static int64_t timeMilli() { return timeMicro()/1000; }
    static int64_t steadyMicro();
    static int64_t steadyMilli() { return steadyMicro()/1000; }
    static std::string readableTime(time_t t);
    static int64_t atoi(const char* b, const char* e) { return strtol(b, (char**)&e, 10); }
    static int64_t atoi2(const char* b, const char* e) {
        char** ne = (char**)&e;
        int64_t v = strtol(b, ne, 10);
        return ne == (char**)&e ? v : -1;
    }
    static int64_t atoi(const char* b) { return atoi(b, b+strlen(b)); }
    static int addFdFlag(int fd, int flag);
};

struct ExitCaller: private noncopyable {
    ~ExitCaller() { functor_(); }
    ExitCaller(std::function<void()>&& functor): functor_(std::move(functor)) {}
private:
    std::function<void()> functor_;
};

}
