#include "handy.h"
#include "logging.h"
#include "util.h"
#include <map>
#include <string.h>
#include <fcntl.h>
#include "poller.h"
#include "conn.h"
namespace handy {

#ifdef OS_LINUX

PollerEpoll::PollerEpoll(){
    fd_ = epoll_create1(EPOLL_CLOEXEC);
    fatalif(fd_<0, "epoll_create error %d %s", errno, strerror(errno));
    info("poller epoll %d created", fd_);
}

PollerEpoll::~PollerEpoll() {
    info("destroying poller %d", fd_);
    while (liveChannels_.size()) {
        (*liveChannels_.begin())->close();
    }
    ::close(fd_);
    info("poller %d destroyed", fd_);
}

void PollerEpoll::addChannel(Channel* ch) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = ch->events();
    ev.data.ptr = ch;
    trace("adding channel %lld fd %d events %d epoll %d", (long long)ch->id(), ch->fd(), ev.events, fd_);
    int r = epoll_ctl(fd_, EPOLL_CTL_ADD, ch->fd(), &ev);
    fatalif(r, "epoll_ctl add failed %d %s", errno, strerror(errno));
    liveChannels_.insert(ch);
}

void PollerEpoll::updateChannel(Channel* ch) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = ch->events();
    ev.data.ptr = ch;
    trace("modifying channel %lld fd %d events read %d write %d epoll %d",
          (long long)ch->id(), ch->fd(), ev.events & POLLIN, ev.events & POLLOUT, fd_);
    int r = epoll_ctl(fd_, EPOLL_CTL_MOD, ch->fd(), &ev);
    fatalif(r, "epoll_ctl mod failed %d %s", errno, strerror(errno));
}

void PollerEpoll::removeChannel(Channel* ch) {
    trace("deleting channel %lld fd %d epoll %d", (long long)ch->id(), ch->fd(), fd_);
    liveChannels_.erase(ch);
    for (int i = lastActive_; i >= 0; i --) {
        if (ch == activeEvs_[i].data.ptr) {
            activeEvs_[i].data.ptr = NULL;
            break;
        }
    }
}

void PollerEpoll::loop_once(int waitMs) {
    long ticks = util::timeMilli();
    lastActive_ = epoll_wait(fd_, activeEvs_, kMaxEvents, waitMs);
    trace("epoll wait %d return %d used %lld millsecond",
          waitMs, lastActive_, util::timeMilli()-ticks);
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
                fatal("unexpected poller events");
            }
        }
    }
}

#elif defined(OS_MACOSX)

PollerKqueue::PollerKqueue(){
    fd_ = kqueue();
    fatalif(fd_ <0, "kqueue error %d %s", errno, strerror(errno));
    info("poller kqueue %d created", fd_);
}

PollerKqueue::~PollerKqueue() {
    info("destroying poller %d", fd_);
    while (liveChannels_.size()) {
        (*liveChannels_.begin())->close();
    }
    ::close(fd_);
    info("poller %d destroyed", fd_);
}

// if read and write are enabled, only monitor write event
static int getKqueueEvent(int event) {
    return event & kWriteEvent ? EVFILT_WRITE : EVFILT_READ;
}

void PollerKqueue::addChannel(Channel* ch) {
    struct timespec now;
    now.tv_nsec = 0;
    now.tv_sec = 0;
    struct kevent ev;
    int kev = getKqueueEvent(ch->events());
    EV_SET(&ev, ch->fd(), kev, EV_ADD|EV_ENABLE, 0, 0, ch);
    trace("adding channel %lld fd %d events read %d write %d  epoll %d",
          (long long)ch->id(), ch->fd(), ch->events() & POLLIN, ch->events() & POLLOUT, fd_);
    int r = kevent(fd_, &ev, 1, NULL, 0, &now);
    fatalif(r, "kevent add failed %d %s", errno, strerror(errno));
    liveChannels_.insert(ch);
}

void PollerKqueue::updateChannel(Channel* ch) {
    struct timespec now;
    now.tv_nsec = 0;
    now.tv_sec = 0;
    struct kevent ev;
    int kev = getKqueueEvent(ch->events());
    EV_SET(&ev, ch->fd(), kev, EV_ADD|EV_ENABLE, 0, 0, ch);
    trace("modifying channel %lld fd %d events read %d write %d epoll %d",
          (long long)ch->id(), ch->fd(), ch->events() & POLLIN, ch->events() & POLLOUT, fd_);
    int r = kevent(fd_, &ev, 1, NULL, 0, &now);
    fatalif(r, "kevent mod failed %d %s", errno, strerror(errno));
}

void PollerKqueue::removeChannel(Channel* ch) {
    trace("deleting channel %lld fd %d epoll %d", (long long)ch->id(), ch->fd(), fd_);
    liveChannels_.erase(ch);
    // remove channel if in ready stat
    for (int i = lastActive_; i >= 0; i --) {
        if (ch == activeEvs_[i].udata) {
            activeEvs_[i].udata = NULL;
            break;
        }
    }
}

void PollerKqueue::loop_once(int waitMs) {
    struct timespec timeout;
    timeout.tv_sec = waitMs / 1000;
    timeout.tv_nsec = (waitMs % 1000) * 1000 * 1000;
    long ticks = util::timeMilli();
    lastActive_ = kevent(fd_, NULL, 0, activeEvs_, kMaxEvents, &timeout);
    trace("kevent wait %d return %d used %lld millsecond",
          waitMs, lastActive_, util::timeMilli()-ticks);
    fatalif(lastActive_ == -1, "kevent return error %d %s", errno, strerror(errno));
    while (--lastActive_ >= 0) {
        int i = lastActive_;
        Channel* ch = (Channel*)activeEvs_[i].udata;
        struct kevent& ke = activeEvs_[i];
        if (ch) {
            // only handle write if read and write are enabled
            if (!(ke.flags & EV_EOF) && ch->writeEnabled()) {
                trace("channel %lld fd %d handle write", (long long)ch->id(), ch->fd());
                ch->handleWrite();
            } else if ((ke.flags & EV_EOF) || ch->readEnabled()) {
                trace("channel %lld fd %d handle read", (long long)ch->id(), ch->fd());
                ch->handleRead();
            } else {
                fatal("unexpected epoll events %d", ch->events());
            }
        }
    }
}

#endif
}