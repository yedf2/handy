#pragma once
#include "util.h"
#include "net.h"
#include "thread_util.h"
#include <utility>
#include <set>
#include <memory>
#include <unistd.h>

namespace handy {
struct Channel;
class TcpConn;
struct IdleIdImp;
struct EventsImp;
typedef std::unique_ptr<IdleIdImp> IdleId;

struct AutoContext {
    void* ctx;
    Task ctxDel;
    AutoContext():ctx(0) {}
    template<class T> T& context() {
        if (ctx == NULL) {
            ctx = new T();
            ctxDel = [this] { delete (T*)ctx; };
        }
        return *(T*)ctx;
    }
    ~AutoContext() { if (ctx) ctxDel(); }
};

}