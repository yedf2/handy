#pragma once
#include <string>
#include <functional>
#include <utility>
#include <vector>

namespace handy {

namespace {
    int16_t htobe(int16_t v) { return htobe16(v); }
    int32_t htobe(int32_t v) { return htobe32(v); }
    int64_t htobe(int64_t v) { return htobe64(v); }
    uint16_t htobe(uint16_t v) { return htobe16(v); }
    uint32_t htobe(uint32_t v) { return htobe32(v); }
    uint64_t htobe(uint64_t v) { return htobe64(v); }
}

struct util {
    static std::string format(const char* fmt, ...);
    static int setNonBlock(int fd);
    static int setReuseAddr(int fd);
    static int64_t timeMicro();
    static int64_t timeMilli() { return timeMicro()/1000; }
    static int64_t steadyMicro();
    static int64_t steadyMilli() { return steadyMicro()/1000; }
    template<class T> T hton(T v) { return htobe(v); }
    template<class T> T ntoh(T v) { return htobe(v); }
};

struct ExitCaller {
    ~ExitCaller() { functor_(); }
    ExitCaller(std::function<void()>&& functor): functor_(std::move(functor)) {}
private:
    std::function<void()> functor_;
};

}
