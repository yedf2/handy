#pragma once
#include "handy-imp.h"
#include "poller.h"

namespace handy {

typedef std::shared_ptr<TcpConn> TcpConnPtr;
typedef std::shared_ptr<TcpServer> TcpServerPtr;
typedef std::function<void(const TcpConnPtr&)> TcpCallBack;
typedef std::function<void(const TcpConnPtr&, Slice msg)> MsgCallBack;

struct EventBases {
    virtual EventBase* allocBase() = 0;
};

//Event dispatcher, manageable timers, connections, timeout connections.
struct EventBase: public EventBases {
    //The taskCapacity specifies the size of the task queue, unlimited if `taskCapacity` == 0.
    EventBase(int taskCapacity=0);
    ~EventBase();
    //Handling expired events, `waitMs` indicates the time to wait if there are no tasks currently to be processed.
    void loop_once(int waitMs);
    //Enter event processing loop.
    void loop() ;
    //Cancel the scheduled task, ignore if the timer has expired.
    bool cancel(TimerId timerid);
    //Add a timed task, if `interval` == 0 means a one-time task, otherwise it is a repeating task, the time is milliseconds.
    TimerId runAt(int64_t milli, const Task& task, int64_t interval=0) { return runAt(milli, Task(task), interval); }
    TimerId runAt(int64_t milli, Task&& task, int64_t interval=0);
    TimerId runAfter(int64_t milli, const Task& task, int64_t interval=0) { return runAt(util::timeMilli()+milli, Task(task), interval); }
    TimerId runAfter(int64_t milli, Task&& task, int64_t interval=0) { return runAt(util::timeMilli()+milli, std::move(task), interval);}

    //The following functions are thread safe.

    //Exit event loop.
    EventBase& exit();
    //If has quitted the loop.
    bool exited();
    //Wakeup event handler.
    void wakeup();
    //Add a `task`.
    void safeCall(Task&& task);
    void safeCall(const Task& task) { safeCall(Task(task)); }
    //Assign an event dispatcher.
    virtual EventBase* allocBase() { return this; }

public:
    std::unique_ptr<EventsImp> imp_;
};

//Multi-threaded event dispatcher.
struct MultiBase: public EventBases{
    MultiBase(int sz): id_(0), bases_(sz) {}
    virtual EventBase* allocBase() { int c = id_++; return &bases_[c%bases_.size()]; }
    void loop();
    MultiBase& exit() { for (auto& b: bases_) { b.exit(); } return *this; }
private:
    std::atomic<int> id_;
    std::vector<EventBase> bases_;
};

//`Channel` which encapsulates an fd that can be epoll.
struct Channel {
    //The `base` is the event manager, `fd` is the fd inside the channel, and `events` is the event of interest to the channel.
    Channel(EventBase* base, int fd, int events);
    ~Channel();
    EventBase* getBase() { return base_; }
    int fd() { return fd_; }
    //The id of channel.
    int64_t id() { return id_; }
    short events() { return events_; }
    //Close the channel.
    void close();

    //Mount time processor.
    void onRead(const Task& readcb) { readcb_ = readcb; }
    void onWrite(const Task& writecb) { writecb_ = writecb; }
    void onRead(Task&& readcb) { readcb_ = std::move(readcb); }
    void onWrite(Task&& writecb) { writecb_ = std::move(writecb); }

    //Enable read and write listening.
    void enableRead(bool enable);
    void enableWrite(bool enable);
    void enableReadWrite(bool readable, bool writable);
    bool readEnabled();
    bool writeEnabled();

    //Handling read and write events.
    void handleRead() { readcb_(); }
    void handleWrite() { writecb_(); }
protected:
    EventBase* base_;
    PollerBase* poller_;
    int fd_;
    short events_;
    int64_t id_;
    std::function<void()> readcb_, writecb_, errorcb_;
};

}
