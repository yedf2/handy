#pragma once
#include <stdlib.h>
#include <string.h>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace handy {

struct noncopyable {
   protected:
    noncopyable() = default;
    virtual ~noncopyable() = default;

    noncopyable(const noncopyable &) = delete;
    noncopyable &operator=(const noncopyable &) = delete;
};

struct util {
    static std::string format(const char *fmt, ...);
    static int64_t timeMicro();
    static int64_t timeMilli() { return timeMicro() / 1000; }
    static int64_t steadyMicro();
    static int64_t steadyMilli() { return steadyMicro() / 1000; }
    static std::string readableTime(time_t t);
    static int64_t atoi(const char *b, const char *e) { return strtol(b, (char **) &e, 10); }
    static int64_t atoi2(const char *b, const char *e) {
        char* ne = nullptr;
        int64_t v = strtol(b, &ne, 10);
        return ne == e ? v : -1;
    }
    static int64_t atoi(const char *b) { return atoi(b, b + strlen(b)); }
    static int addFdFlag(int fd, int flag);
};

struct ExitCaller : private noncopyable {
    ~ExitCaller() { functor_(); }
    ExitCaller(std::function<void()> &&functor) : functor_(std::move(functor)) {}

   private:
    std::function<void()> functor_;
};

}  // namespace handy
