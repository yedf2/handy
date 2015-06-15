#pragma once
#include <poll.h>
#include <assert.h>
#include <map>
#include <atomic>
namespace handy {

const int kReadEvent = POLLIN;
const int kWriteEvent = POLLOUT;
const int kMaxEvents = 2000;

struct PollerBase {
    int64_t id_;
    int lastActive_;
    PollerBase(): lastActive_(-1) { static std::atomic<int64_t> id(0); id_ = ++id; }
    virtual void addChannel(Channel* ch) = 0;
    virtual void removeChannel(Channel* ch) = 0;
    virtual void updateChannel(Channel* ch) = 0;
    virtual void loop_once(int waitMs) = 0;
    virtual ~PollerBase(){};
};

struct PollerPoll : PollerBase {
    std::map<Channel*, size_t> liveChannels_;
    std::map<int, Channel*> fdChannels_;
    std::vector<struct pollfd> fds_;
    virtual void addChannel(Channel* ch) override;
    virtual void removeChannel(Channel* ch) override;
    virtual void updateChannel(Channel* ch) override;
    virtual void loop_once(int waitMs) override;
    virtual ~PollerPoll() override;
};

#ifdef USE_EPOLL

#include <sys/epoll.h>

struct PollerEpoll : public PollerPoll {
    int epollfd_;
    std::set<Channel*> liveChannels_;
    //for epoll selected active events
    struct epoll_event activeEvs_[kMaxEvents];
    PollerEpoll();
    void init();
    ~PollerEpoll();
    void addChannel(Channel* ch) override;
    void removeChannel(Channel* ch) override;
    void updateChannel(Channel* ch) override;
    void loop_once(int waitMs) override;
};

#define PlatformPoller PollerEpoll
#else
#define PlatformPoller PollerPoll
#endif
}