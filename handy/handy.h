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
typedef std::pair<int64_t, int64_t> TimerId;

class TcpConn;

typedef std::shared_ptr<TcpConn> TcpConnPtr;

typedef std::function<void(const TcpConnPtr&)> TcpCallBack;

struct IdleIdImp;
typedef std::unique_ptr<IdleIdImp> IdleId;

struct TimerImp;

struct EventBase {
    static const int kReadEvent;
    static const int kWriteEvent;

    EventBase(int maxTasks=0);
    ~EventBase();
    void loop_once(int waitMs);
    void addChannel(Channel* ch);
    void removeChannel(Channel* ch);
    void updateChannel(Channel* ch);
    void loop() { while (!exit_) loop_once(10000); loop_once(0); }
    bool cancel(TimerId timerid);
    TimerId runAt(int64_t milli, const Task& task, int64_t interval=0) { return runAt(milli, Task(task), interval); }
    TimerId runAt(int64_t milli, Task&& task, int64_t interval=0);
    TimerId runAfter(int64_t milli, const Task& task, int64_t interval=0) { return runAt(util::timeMilli()+milli, Task(task), interval); }
    TimerId runAfter(int64_t milli, Task&& task, int64_t interval=0) { return runAt(util::timeMilli()+milli, std::move(task), interval);}

    //following functions is thread safe
    void exit() { exit_ = true; wakeup();}
    void wakeup();
    void safeCall(const Task& task) { safeCall(Task(task)); }
    void safeCall(Task&& task) { tasks_.push(std::move(task)); wakeup(); }
private:
    friend TcpConn;
    std::set<Channel*> live_channels_;
    static const int kMaxEvents = 20;
    std::atomic<bool> exit_;
    int wakeupFds_[2];
    int epollfd;
    SafeQueue<Task> tasks_;
    std::unique_ptr<TimerImp> timerImp_;
};

struct Channel {
    Channel(EventBase* base, int fd, int events);
    int fd() { return fd_; }
    EventBase* getBase() { return base_; }
    ~Channel() { base_->removeChannel(this); ::close(fd_); }
    int events() { return events_; }
    void onRead(const Task& readcb) { readcb_ = readcb; }
    void onWrite(const Task& writecb) { writecb_ = writecb; }
    void onRead(Task&& readcb) { readcb_ = std::move(readcb); }
    void onWrite(Task&& writecb) { writecb_ = std::move(writecb); }
    void handleRead() { readcb_(); }
    void handleWrite() { writecb_(); }
    void enableRead(bool enable);
    void enableWrite(bool enable);
    void enableReadWrite(bool readable, bool writable);
    bool readEnabled() { return events_ & EventBase::kReadEvent; }
    bool writeEnabled() { return events_ & EventBase::kWriteEvent; }
private:
    EventBase* base_;
    int fd_;
    int events_;
    std::function<void()> readcb_, writecb_, errorcb_;
};

struct TcpConn: public std::enable_shared_from_this<TcpConn> {
    enum State { Connecting, Connected, Closed, Failed, };
    static TcpConnPtr create(EventBase* base, int fd, Ip4Addr local, Ip4Addr peer);
    static TcpConnPtr connectTo(EventBase* base, Ip4Addr addr);
    static TcpConnPtr connectTo(EventBase* base, const char* host, short port) { return connectTo(base, Ip4Addr(host, port)); }
    ~TcpConn();
    //automatically managed context. allocated when first used, deleted when destruct
    template<class T> T& context();
    EventBase* getBase() { return channel_ ? channel_->getBase() : NULL; }
    State getState() { return state_; }
    bool writable() { return channel_ ? !channel_->writeEnabled(): false; }
    void sendOutput() { send(output_); }
    void send(Buffer& msg);
    void send(const char* buf, size_t len);
    void send(const std::string& s) { send(s.data(), s.size()); }
    void send(const char* s) { send(s, strlen(s)); }
    void onRead(const TcpCallBack& cb) { readcb_ = cb; };
    void onWritable(const TcpCallBack& cb) { writablecb_ = cb;}
    void onState(const TcpCallBack& cb) { statecb_ = cb; }
    void onIdle(int idle, const TcpCallBack& cb) { onIdle(idle, TcpCallBack(cb)); }
    void onRead(TcpCallBack&& cb) { readcb_ = std::move(cb); };
    void onWritable(TcpCallBack&& cb) { writablecb_ = std::move(cb);}
    void onState(TcpCallBack&& cb) { statecb_ = std::move(cb); }
    void onIdle(int idle, TcpCallBack&& cb);
    void close();
    Buffer& getInput() { return input_; }
    Buffer& getOutput() { return output_; }
protected:
    TcpConn(EventBase* base, int fd, Ip4Addr local, Ip4Addr peer);
    Channel* channel_;
    Buffer input_, output_;
    Ip4Addr local_, peer_;
    void* ctx_;
    std::function<void()> ctxDel_;
    State state_;
    TcpCallBack readcb_, writablecb_, statecb_;
    IdleId idleId_;
    void handleRead(const TcpConnPtr& con);
    void handleWrite(const TcpConnPtr& con);
    ssize_t isend(const char* buf, size_t len);
};

struct TcpServer {
    //abort if bind failed
    TcpServer(EventBase* base, Ip4Addr addr);
    TcpServer(EventBase* base, short port): TcpServer(base, Ip4Addr(port)) {}
    TcpServer(EventBase* base, const char* host, short port): TcpServer(base, Ip4Addr(host, port)) {}
    ~TcpServer() { delete listen_channel_; }
    Ip4Addr getAddr() { return addr_; }
    //these functions are not called frequently, so "move" functions are not provided
    void onConnRead(const TcpCallBack& cb) { readcb_ = cb; }
    void onConnWritable(const TcpCallBack& cb) { writablecb_ = cb; }
    void onConnState(const TcpCallBack& cb) { statecb_ = cb; }
    void onConnIdle(int idle, const TcpCallBack& cb) { idle_ = idle; idlecb_ = cb; }
private:
    EventBase* base_;
    Ip4Addr addr_;
    Channel* listen_channel_;
    int idle_;
    TcpCallBack readcb_, writablecb_, statecb_, idlecb_;
    void handleAccept();
};

template<class T> T& TcpConn::context() {
    if (ctx_ == NULL) {
        ctx_ = new T();
        ctxDel_ = [this] { delete (T*)ctx_; };
    }
    return *(T*)ctx_;
}

}