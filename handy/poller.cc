#include "handy.h"
#include "logging.h"
#include "util.h"
#include <map>
#include <string.h>
#include <fcntl.h>
#include "poller.h"
#include "conn.h"
namespace handy {

void PollerPoll::addChannel(Channel* ch) {
    struct pollfd ev;
    ev.events = ch->events();
    ev.fd = ch->fd();
    liveChannels_[ch] = fds_.size();
    fdChannels_[ch->fd()] = ch;
    fds_.push_back(ev);
    trace("adding channel %lld fd %d events %d poll %lld", (long long)ch->id(), ch->fd(), ev.events, (long long)id_);
}

void PollerPoll::updateChannel(Channel* ch) {
    if (ch->fd() < 0) {
        return;
    }
    auto iter = liveChannels_.find(ch);
    assert(iter != liveChannels_.end());
    struct pollfd& ev = fds_[iter->second];
    ev.events = ch->events();
    trace("modifying channel %lld fd %d events read %d write %d epoll %lld",
          (long long)ch->id(), ch->fd(), ev.events & POLLIN, ev.events & POLLOUT, (long long)id_);
}

void PollerPoll::removeChannel(Channel* ch) {
    trace("deleting channel %lld fd %d epoll %lld", (long long)ch->id(), ch->fd(), (long long)id_);
    auto iter = liveChannels_.find(ch);
    assert(iter != liveChannels_.end());
    struct pollfd& ev = fds_[iter->second];
    fdChannels_.erase(ev.fd);
    std::iter_swap(fds_.begin() + iter->second, --fds_.end());
    fds_.pop_back();
    liveChannels_.erase(ch);
    //TODO remove of waitting to be processed
}

void PollerPoll::loop_once(int waitMs) {
    int n = ::poll(&fds_[0], (nfds_t)fds_.size(), waitMs);
    lastActive_ = (int)fds_.size();
    while (--lastActive_ >= 0) {
        int i = lastActive_;
        int events = fds_[i].revents;
        if (events) {
            auto iter = fdChannels_.find(fds_[i].fd);
            assert(iter != fdChannels_.end());
            Channel* ch = iter->second;
            if (events & (kReadEvent | POLLERR)) {
                trace("channel %lld fd %d handle read", (long long)ch->id(), ch->fd());
                ch->handleRead();
            } else if (events & kWriteEvent) {
                trace("channel %lld fd %d handle write", (long long)ch->id(), ch->fd());
                ch->handleWrite();
            } else {
                fatal("unexpected poll events");
            }
        }
    }
}

PollerPoll::~PollerPoll() {
    info("destroying PollerPoll %lld", (long long)id_);
    for (auto ch: liveChannels_) {
        ch.first->close();
    }
    while (liveChannels_.size()) {
        liveChannels_.begin()->first->handleRead();
    }
}


#ifdef USE_EPOLL

void PollerEpoll::init() {
}

PollerEpoll::PollerEpoll(){
    assert(POLLIN == EPOLLIN);
    assert(POLLOUT == EPOLLOUT);
    assert(POLLERR == EPOLLERR);
    epollfd_ = epoll_create1(EPOLL_CLOEXEC);
    fatalif(epollfd_<0, "epoll_create error %d %s", errno, strerror(errno));
    info("event base %d created", epollfd_);
}

PollerEpoll::~PollerEpoll() {
    ::close(epollfd_);
    info("event base %d destroyed", epollfd_);
}

void PollerEpoll::addChannel(Channel* ch) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = ch->events();
    ev.data.ptr = ch;
    trace("adding channel %lld fd %d events %d epoll %d", (long long)ch->id(), ch->fd(), ev.events, epollfd_);
    int r = epoll_ctl(epollfd_, EPOLL_CTL_ADD, ch->fd(), &ev);
    fatalif(r, "epoll_ctl add failed %d %s", errno, strerror(errno));
    liveChannels_.insert(ch);
}

void PollerEpoll::updateChannel(Channel* ch) {
    if (ch->fd() < 0) {
        return;
    }
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = ch->events();
    ev.data.ptr = ch;
    trace("modifying channel %lld fd %d events read %d write %d epoll %d",
          (long long)ch->id(), ch->fd(), ev.events & POLLIN, ev.events & POLLOUT, epollfd_);
    int r = epoll_ctl(epollfd_, EPOLL_CTL_MOD, ch->fd(), &ev);
    fatalif(r, "epoll_ctl mod failed %d %s", errno, strerror(errno));
}

void PollerEpoll::removeChannel(Channel* ch) {
    liveChannels_.erase(ch);
    for (int i = lastActive_; i >= 0; i --) {
        if (ch == activeEvs_[i].data.ptr) {
            activeEvs_[i].data.ptr = NULL;
            break;
        }
    }
    trace("deleting channel %lld fd %d epoll %d", (long long)ch->id(), ch->fd(), epollfd_);
    if (ch->fd() < 0) {
        return;
    }
    int r = epoll_ctl(epollfd_, EPOLL_CTL_DEL, ch->fd(), NULL);
    fatalif(r && errno != EBADF, "epoll_ctl fd: %d del failed %d %s", ch->fd(), errno, strerror(errno));
}

void PollerEpoll::loop_once(int waitMs) {
    lastActive_ = epoll_wait(epollfd_, activeEvs_, kMaxEvents, waitMs);
    while (--lastActive_ >= 0) {
        int i = lastActive_;
        Channel* ch = (Channel*)activeEvs_[i].data.ptr;
        int events = activeEvs_[i].events;
        if (ch) {
            if (events & (kReadEvent | POLLERR)) {
                trace("channel %lld fd %d handle read", (long long)ch->id(), ch->fd());
                ch->handleRead();
            } else if (events & kWriteEvent) {
                trace("channel %lld fd %d handle write", (long long)ch->id(), ch->fd());
                ch->handleWrite();
            } else {
                fatal("unexpected epoll events");
            }
        }
    }
}


#endif
}