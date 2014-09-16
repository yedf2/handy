#pragma once
#include "handy_imp.h"

namespace handy {

typedef std::pair<int64_t, int64_t> TimerId;

typedef std::shared_ptr<TcpConn> TcpConnPtr;

typedef std::function<void(const TcpConnPtr&)> TcpCallBack;

struct EventBase {
    EventBase(int taskCapacity=0);
    ~EventBase();
    void loop_once(int waitMs);
    void loop() ;
    bool cancel(TimerId timerid);
    TimerId runAt(int64_t milli, const Task& task, int64_t interval=0) { return runAt(milli, Task(task), interval); }
    TimerId runAt(int64_t milli, Task&& task, int64_t interval=0);
    TimerId runAfter(int64_t milli, const Task& task, int64_t interval=0) { return runAt(util::timeMilli()+milli, Task(task), interval); }
    TimerId runAfter(int64_t milli, Task&& task, int64_t interval=0) { return runAt(util::timeMilli()+milli, std::move(task), interval);}

    //following functions is thread safe
    EventBase& exit();
    bool exited();
    void wakeup();
    void safeCall(Task&& task);
    void safeCall(const Task& task) { safeCall(Task(task)); }
private:
    friend TcpConn;
    friend Channel;
    std::unique_ptr<EventsImp> imp_;
};

struct MultiBase {
    MultiBase(int sz): id_(0), bases_(sz) {}
    EventBase* allocBase() { int c = id_++; return &bases_[c%bases_.size()]; }
    void loop();
    MultiBase& exit() { for (auto& b: bases_) { b.exit(); } return *this; }
private:
    std::atomic<int> id_;
    std::vector<EventBase> bases_;
};

struct Channel {
    Channel(EventBase* base, int fd, int events);
    ~Channel();
    EventBase* getBase() { return base_; }
    int fd() { return fd_; }
    int64_t id() { return id_; }
    int events() { return events_; }
    void close();
    void onRead(const Task& readcb) { readcb_ = readcb; }
    void onWrite(const Task& writecb) { writecb_ = writecb; }
    void onRead(Task&& readcb) { readcb_ = std::move(readcb); }
    void onWrite(Task&& writecb) { writecb_ = std::move(writecb); }
    void enableRead(bool enable);
    void enableWrite(bool enable);
    void enableReadWrite(bool readable, bool writable);
    bool readEnabled();
    bool writeEnabled();

    void handleRead() { readcb_(); }
    void handleWrite() { writecb_(); }
protected:
    friend EventsImp;
    EventBase* base_;
    int fd_;
    int events_;
    int64_t id_;
    std::list<Channel*>::iterator eventPos_;
    std::function<void()> readcb_, writecb_, errorcb_;
};

struct TcpConn: public std::enable_shared_from_this<TcpConn> {
    enum State { Connecting, Connected, Closed, Failed, };
    static TcpConnPtr create(EventBase* base, int fd, Ip4Addr local, Ip4Addr peer);
    static TcpConnPtr connectTo(EventBase* base, Ip4Addr addr, int timeout=0);
    static TcpConnPtr connectTo(EventBase* base, const std::string& host, short port, int timeout=0) { return connectTo(base, Ip4Addr(host, port), timeout); }

    ~TcpConn();

    //automatically managed context. allocated when first used, deleted when destruct
    template<class T> T& context() { return ctx_.context<T>(); }

    EventBase* getBase() { return channel_ ? channel_->getBase() : NULL; }
    State getState() { return state_; }
    Buffer& getInput() { return input_; }
    Buffer& getOutput() { return output_; }
    bool writable() { return channel_ ? !channel_->writeEnabled(): false; }

    void sendOutput() { send(output_); }
    void send(Buffer& msg);
    void send(const char* buf, size_t len);
    void send(const std::string& s) { send(s.data(), s.size()); }
    void send(const char* s) { send(s, strlen(s)); }

    void onRead(const TcpCallBack& cb) { readcb_ = cb; };
    void onWritable(const TcpCallBack& cb) { writablecb_ = cb;}
    void onState(const TcpCallBack& cb) { statecb_ = cb; }
    void addIdleCB(int idle, const TcpCallBack& cb);

    void close(bool cleanupNow=false);
protected:
    TcpConn(EventBase* base, int fd, Ip4Addr local, Ip4Addr peer);
    Channel* channel_;
    Buffer input_, output_;
    Ip4Addr local_, peer_;
    State state_;
    TcpCallBack readcb_, writablecb_, statecb_;
    std::list<IdleId> idleIds_;
    AutoContext ctx_, internalCtx_;
    void handleRead(const TcpConnPtr& con);
    void handleWrite(const TcpConnPtr& con);
    ssize_t isend(const char* buf, size_t len);
};

struct TcpServer {
    //abort if bind failed
    TcpServer(EventBase* base, Ip4Addr addr);
    TcpServer(EventBase* base, const std::string& host, short port): TcpServer(base, Ip4Addr(host, port)) {}
    TcpServer(MultiBase* bases, Ip4Addr addr):TcpServer(bases->allocBase(), addr) { bases_ = [bases] {return bases->allocBase(); }; }
    TcpServer(MultiBase* bases, const std::string& host, short port): TcpServer(bases, Ip4Addr(host, port)) {}
    ~TcpServer() { delete listen_channel_; }
    Ip4Addr getAddr() { return addr_; }
    //these functions are not called frequently, so "move" functions are not provided
    void onConnRead(const TcpCallBack& cb) { readcb_ = cb; }
    void onConnWritable(const TcpCallBack& cb) { writablecb_ = cb; }
    void onConnState(const TcpCallBack& cb) { statecb_ = cb; }
    void onConnIdle(int idle, const TcpCallBack& cb) { idles_.push_back({idle, cb}); }
private:
    EventBase* base_;
    std::function<EventBase*()> bases_;
    Ip4Addr addr_;
    Channel* listen_channel_;
    typedef std::pair<int, TcpCallBack> IdlePair;
    std::list<IdlePair> idles_;
    TcpCallBack readcb_, writablecb_, statecb_;
    void handleAccept();
};

}