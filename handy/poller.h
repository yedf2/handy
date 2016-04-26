#pragma once
#include <poll.h>
#include <assert.h>
#include <map>
#include <atomic>
#include <sys/time.h>
#include <sys/types.h>

#ifdef OS_LINUX
#include <sys/epoll.h>
#elif defined(OS_MACOSX)
#include <sys/event.h>
#else
#error "platform unsupported"
#endif

namespace handy {

const int kMaxEvents = 2000;

struct PollerBase: private noncopyable {
    int64_t id_;
    int lastActive_;
    PollerBase(): lastActive_(-1) { static std::atomic<int64_t> id(0); id_ = ++id; }
    virtual void addChannel(Channel* ch) = 0;
    virtual void removeChannel(Channel* ch) = 0;
    virtual void updateChannel(Channel* ch) = 0;
    virtual void loop_once(int waitMs) = 0;
    virtual ~PollerBase(){};
};

#ifdef OS_LINUX

const int kReadEvent = EPOLLIN;
const int kWriteEvent = EPOLLOUT;

struct PollerEpoll : public PollerBase{
    int fd_;
    std::set<Channel*> liveChannels_;
    //for epoll selected active events
    struct epoll_event activeEvs_[kMaxEvents];
    PollerEpoll();
    ~PollerEpoll();
    void addChannel(Channel* ch) override;
    void removeChannel(Channel* ch) override;
    void updateChannel(Channel* ch) override;
    void loop_once(int waitMs) override;
};

#define PlatformPoller PollerEpoll

#elif defined(OS_MACOSX)

const int kReadEvent = POLL_IN;
const int kWriteEvent = POLL_OUT;

    struct PollerKqueue : public PollerBase {
        int fd_;
        std::set<Channel*> liveChannels_;
        //for epoll selected active events
        struct kevent activeEvs_[kMaxEvents];
        PollerKqueue();
        ~PollerKqueue();
        void addChannel(Channel* ch) override;
        void removeChannel(Channel* ch) override;
        void updateChannel(Channel* ch) override;
        void loop_once(int waitMs) override;
    };

#define PlatformPoller PollerKqueue

#else
#error "platform not supported"
#endif
}