#include "port_posix.h"
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>

namespace handy {
namespace port{
#ifdef OS_LINUX
    struct in_addr getHostByName(const std::string& host) {
        struct in_addr addr;
        char buf[1024];
        struct hostent hent;
        struct hostent* he = NULL;
        int herrno = 0;
        memset(&hent, 0, sizeof hent);
        int r = gethostbyname_r(host.c_str(), &hent, buf, sizeof buf, &he, &herrno);
        if (r == 0 && he && he->h_addrtype==AF_INET) {
            addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
        } else {
            addr.s_addr = INADDR_NONE;
        }
        return addr;
    }
    uint64_t gettid() { return syscall(SYS_gettid); }
#elif defined(OS_MACOSX)
    struct in_addr getHostByName(const std::string& host) {
        struct in_addr addr;
        struct hostent* he = gethostbyname(host.c_str());
        if (he && he->h_addrtype == AF_INET) {
            addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
        } else {
            addr.s_addr = INADDR_NONE;
        }
        return addr;
    }
    uint64_t gettid() {
        pthread_t tid = pthread_self();
        uint64_t uid = 0;
        memcpy(&uid, &tid, std::min(sizeof(tid), sizeof(uid)));
        return uid;
    }
#endif

}
}