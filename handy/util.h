#pragma once
#include <string>
#include <functional>
#include <utility>
#include <vector>
#include <string.h>

namespace handy {

struct util {
    static std::string format(const char* fmt, ...);
    static int64_t timeMicro();
    static int64_t timeMilli() { return timeMicro()/1000; }
    static int64_t steadyMicro();
    static int64_t steadyMilli() { return steadyMicro()/1000; }
    static std::string readableTime(time_t t);
    static int64_t atoi(const char* b, const char* e) { return strtol(b, (char**)&e, 10); }
    static int64_t atoi(const char* b) { return atoi(b, b+strlen(b)); }
};

struct ExitCaller {
    ~ExitCaller() { functor_(); }
    ExitCaller(std::function<void()>&& functor): functor_(std::move(functor)) {}
private:
    std::function<void()> functor_;
};

}
