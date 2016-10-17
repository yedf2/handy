#include "net.h"
#include "util.h"
#include "logging.h"
#include <netinet/tcp.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <string>

using namespace std;
namespace handy {

int net::setNonBlock(int fd, bool value) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return errno;
    }
    if (value) {
        return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
    return fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}

int net::setReuseAddr(int fd, bool value) {
    int flag = value;
    int len = sizeof flag;
    return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, len);
}

int net::setReusePort(int fd, bool value) {
#ifndef SO_REUSEPORT
    fatalif(value, "SO_REUSEPORT not supported");
    return 0;
#else
    int flag = value;
    int len = sizeof flag;
    return setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &flag, len);
#endif
}

int net::setNoDelay(int fd, bool value) {
    int flag = value;
    int len = sizeof flag;
    return setsockopt(fd, SOL_SOCKET, TCP_NODELAY, &flag, len);
}

Ip4Addr::Ip4Addr(const string& host, short port) {
    memset(&addr_, 0, sizeof addr_);
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    if (host.size()) {
        addr_.sin_addr = port::getHostByName(host);
    } else {
        addr_.sin_addr.s_addr = INADDR_ANY;
    }
    if (addr_.sin_addr.s_addr == INADDR_NONE){
        error("cannot resove %s to ip", host.c_str());
    }
}

string Ip4Addr::toString() const {
    uint32_t uip = addr_.sin_addr.s_addr;
    return util::format("%d.%d.%d.%d:%d",
        (uip >> 0)&0xff,
        (uip >> 8)&0xff,
        (uip >> 16)&0xff,
        (uip >> 24)&0xff,
        ntohs(addr_.sin_port));
}

string Ip4Addr::ip() const { 
    uint32_t uip = addr_.sin_addr.s_addr;
    return util::format("%d.%d.%d.%d",
        (uip >> 0)&0xff,
        (uip >> 8)&0xff,
        (uip >> 16)&0xff,
        (uip >> 24)&0xff);
}

short Ip4Addr::port() const {
    return ntohs(addr_.sin_port);
}

unsigned int Ip4Addr::ipInt() const { 
    return ntohl(addr_.sin_addr.s_addr);
}
bool Ip4Addr::isIpValid() const {
    return addr_.sin_addr.s_addr != INADDR_NONE;
}

char* Buffer::makeRoom(size_t len) {
    if (e_ + len <= cap_) {
    } else if (size() + len < cap_ / 2) {
        moveHead();
    } else {
        expand(len);
    }
    return end();
}

void Buffer::expand(size_t len) {
    size_t ncap = std::max(exp_, std::max(2*cap_, size()+len));
    char* p = new char[ncap];
    std::copy(begin(), end(), p);
    e_ -= b_;
    b_ = 0;
    delete[] buf_;
    buf_ = p;
    cap_ = ncap;
}

void Buffer::copyFrom(const Buffer& b) {
    memcpy(this, &b, sizeof b); 
    if (b.buf_) {
        buf_ = new char[cap_]; 
        memcpy(data(), b.begin(), b.size());
    }
}

Buffer& Buffer::absorb(Buffer& buf) { 
    if (&buf != this) {
        if (size() == 0) {
            char b[sizeof buf];
            memcpy(b, this, sizeof b);
            memcpy(this, &buf, sizeof b);
            memcpy(&buf, b, sizeof b);
            std::swap(exp_, buf.exp_); //keep the origin exp_
        } else {
            append(buf.begin(), buf.size());
            buf.clear();
        }
    }
    return *this;
}


}