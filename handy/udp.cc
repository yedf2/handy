#include "udp.h"
#include <fcntl.h>

using namespace std;

namespace handy {

UdpServer::UdpServer(EventBases *bases):
    base_(bases->allocBase()),
    bases_(bases),
    channel_(NULL)
{
}

int UdpServer::bind(const std::string &host, short port, bool reusePort) {
    addr_ = Ip4Addr(host, port);
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    int r = net::setReuseAddr(fd);
    fatalif(r, "set socket reuse option failed");
    r = net::setReusePort(fd, reusePort);
    fatalif(r, "set socket reuse port option failed");
    r = util::addFdFlag(fd, FD_CLOEXEC);
    fatalif(r, "addFdFlag FD_CLOEXEC failed");
    r = ::bind(fd,(struct sockaddr *)&addr_.getAddr(),sizeof(struct sockaddr));
    if (r) {
        close(fd);
        error("bind to %s failed %d %s", addr_.toString().c_str(), errno, strerror(errno));
        return errno;
    }
    net::setNonBlock(fd);
    info("udp fd %d bind to %s", fd, addr_.toString().c_str());
    channel_ = new Channel(base_, fd, kReadEvent);
    channel_->onRead([this]{
        Buffer buf;
        struct sockaddr_in raddr;
        socklen_t rsz = sizeof(raddr);
        if (!channel_ || channel_->fd() < 0) {
            return;
        }
        int fd = channel_->fd();
        ssize_t rn = recvfrom(fd, buf.makeRoom(kUdpPacketSize), kUdpPacketSize, 0, (sockaddr*)&raddr, &rsz);
        if (rn < 0) {
            error("udp %d recv failed: %d %s", fd, errno, strerror(errno));
            return;
        }
        buf.addSize(rn);
        trace("udp %d recv %ld bytes from %s", fd, rn, Ip4Addr(raddr).toString().data());
        this->msgcb_(shared_from_this(), buf, raddr);
    });
    return 0;
}

UdpServerPtr UdpServer::startServer(EventBases* bases, const std::string& host, short port, bool reusePort) {
    UdpServerPtr p(new UdpServer(bases));
    int r = p->bind(host, port, reusePort);
    if (r) {
        error("bind to %s:%d failed %d %s", host.c_str(), port, errno, strerror(errno));
    }
    return r == 0 ? p : NULL;
}

void UdpServer::sendTo(const char* buf, size_t len, Ip4Addr addr) {
    if (!channel_ || channel_->fd() < 0) {
        warn("udp sending %lu bytes to %s after channel closed", len, addr.toString().data());
        return;
    }
    int fd = channel_->fd();
    int wn = ::sendto(fd, buf, len, 0, (sockaddr*)&addr.getAddr(), sizeof(sockaddr));
    if (wn < 0) {
        error("udp %d sendto %s error: %d %s", fd, addr.toString().c_str(), errno, strerror(errno));
        return;
    }
    trace("udp %d sendto %s %d bytes", fd, addr.toString().c_str(), wn);
}

UdpConnPtr UdpConn::createConnection(EventBase* base, const string& host, short port) {
    Ip4Addr addr(host, port);
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    fatalif(fd<0, "socket failed %d %s", errno, strerror(errno));
    net::setNonBlock(fd);
    int t = util::addFdFlag(fd, FD_CLOEXEC);
    fatalif(t, "addFdFlag FD_CLOEXEC failed %d %s", t, strerror(t));
    int r = ::connect(fd, (sockaddr *) &addr.getAddr(), sizeof(sockaddr_in));
    if (r != 0 && errno != EINPROGRESS) {
        error("connect to %s error %d %s", addr.toString().c_str(), errno, strerror(errno));
        return NULL;
    }
    trace("udp fd %d connecting to %s ok", fd, addr.toString().data());
    UdpConnPtr con(new UdpConn);
    con->destHost_ = host;
    con->destPort_ = port;
    con->peer_ = addr;
    con->base_ = base;
    Channel* ch = new Channel(base, fd, kReadEvent);
    con->channel_ = ch;
    ch->onRead([con]{
        if (!con->channel_ || con->channel_->fd() < 0) {
            return con->close();
        }
        Buffer input;
        int fd = con->channel_->fd();
        int rn = ::read(fd, input.makeRoom(kUdpPacketSize), kUdpPacketSize);
        if (rn < 0) {
            error("udp read from %d error %d %s", fd, errno, strerror(errno));
            return;
        }
        trace("udp %d read %d bytes", fd, rn);
        input.addSize(rn);
        con->cb_(con, input);
    });
    return con;
}

void UdpConn::close() {
    if(!channel_)
        return;
    auto p = channel_;
    channel_=NULL;
    base_->safeCall([p](){ delete p; });
}

void UdpConn::send(const char *buf, size_t len) {
    if (!channel_ || channel_->fd() < 0) {
        warn("udp sending %lu bytes to %s after channel closed", len, peer_.toString().data());
        return;
    }
    int fd = channel_->fd();
    int wn = ::write(fd, buf, len);
    if (wn < 0) {
        error("udp %d write error %d %s", fd, errno, strerror(errno));
        return;
    }
    trace("udp %d write %d bytes", fd, wn);
}

HSHAUPtr HSHAU::startServer(EventBase* base, const std::string& host, short port, int threads) {
    HSHAUPtr p = HSHAUPtr(new HSHAU(threads));
    p->server_ = UdpServer::startServer(base, host, port);
    return p->server_ ? p : NULL;
}

void HSHAU::onMsg(const RetMsgUdpCallBack& cb) {
    server_->onMsg([this, cb](const UdpServerPtr& con, Buffer buf, Ip4Addr addr) {
        std::string input(buf.data(), buf.size());
        threadPool_.addTask([=]{
            std::string output = cb(con, input, addr);
            server_->getBase()->safeCall([=] {if (output.size()) con->sendTo(output, addr); });
        });
    });
}


}