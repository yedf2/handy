#pragma once
#include "handy_imp.h"

namespace handy {

typedef std::pair<int64_t, int64_t> TimerId;

typedef std::shared_ptr<TcpConn> TcpConnPtr;

typedef std::function<void(const TcpConnPtr&)> TcpCallBack;

//事件派发器，可管理定时器，连接，超时连接
struct EventBase {
    //taskCapacity指定任务队列的大小，0无限制
    EventBase(int taskCapacity=0);
    ~EventBase();
    //处理已到期的事件,waitMs表示若无当前需要处理的任务，需要等待的时间
    void loop_once(int waitMs);
    //进入事件处理循环
    void loop() ;
    //取消定时任务，若timer已经过期，则忽略
    bool cancel(TimerId timerid);
    //添加定时任务，interval=0表示一次性任务，否则为重复任务，时间为毫秒
    TimerId runAt(int64_t milli, const Task& task, int64_t interval=0) { return runAt(milli, Task(task), interval); }
    TimerId runAt(int64_t milli, Task&& task, int64_t interval=0);
    TimerId runAfter(int64_t milli, const Task& task, int64_t interval=0) { return runAt(util::timeMilli()+milli, Task(task), interval); }
    TimerId runAfter(int64_t milli, Task&& task, int64_t interval=0) { return runAt(util::timeMilli()+milli, std::move(task), interval);}

    //下列函数为线程安全的

    //退出事件循环
    EventBase& exit();
    //是否已退出
    bool exited();
    //唤醒事件处理
    void wakeup();
    //添加任务
    void safeCall(Task&& task);
    void safeCall(const Task& task) { safeCall(Task(task)); }
private:
    friend TcpConn;
    friend Channel;
    std::unique_ptr<EventsImp> imp_;
};

//多线程的时间派发器
struct MultiBase {
    MultiBase(int sz): id_(0), bases_(sz) {}
    EventBase* allocBase() { int c = id_++; return &bases_[c%bases_.size()]; }
    void loop();
    MultiBase& exit() { for (auto& b: bases_) { b.exit(); } return *this; }
private:
    std::atomic<int> id_;
    std::vector<EventBase> bases_;
};

//通道，封装了可以进行epoll的一个fd
struct Channel {
    //base为事件管理器，fd为通道内部的fd，events为通道关心的事件
    Channel(EventBase* base, int fd, int events);
    ~Channel();
    EventBase* getBase() { return base_; }
    int fd() { return fd_; }
    //通道id
    int64_t id() { return id_; }
    int events() { return events_; }
    //关闭通道
    void close();

    //挂接时间处理器
    void onRead(const Task& readcb) { readcb_ = readcb; }
    void onWrite(const Task& writecb) { writecb_ = writecb; }
    void onRead(Task&& readcb) { readcb_ = std::move(readcb); }
    void onWrite(Task&& writecb) { writecb_ = std::move(writecb); }

    //启用读写监听
    void enableRead(bool enable);
    void enableWrite(bool enable);
    void enableReadWrite(bool readable, bool writable);
    bool readEnabled();
    bool writeEnabled();

    //处理读写事件
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

//Tcp连接，使用引用计数
struct TcpConn: public std::enable_shared_from_this<TcpConn> {
    //Tcp连接的四个状态
    enum State { Connecting, Connected, Closed, Failed, };
    //根据已建立的连接创建TcpConn
    static TcpConnPtr create(EventBase* base, int fd, Ip4Addr local, Ip4Addr peer);
    //连接到远程服务器
    static TcpConnPtr connectTo(EventBase* base, Ip4Addr addr, int timeout=0);
    static TcpConnPtr connectTo(EventBase* base, const std::string& host, short port, int timeout=0) { return connectTo(base, Ip4Addr(host, port), timeout); }

    ~TcpConn();

    //automatically managed context. allocated when first used, deleted when destruct
    template<class T> T& context() { return ctx_.context<T>(); }

    EventBase* getBase() { return channel_ ? channel_->getBase() : NULL; }
    State getState() { return state_; }
    //TcpConn的输入输出缓冲区
    Buffer& getInput() { return input_; }
    Buffer& getOutput() { return output_; }
    bool writable() { return channel_ ? channel_->writeEnabled(): false; }

    //发送数据
    void sendOutput() { send(output_); }
    void send(Buffer& msg);
    void send(const char* buf, size_t len);
    void send(const std::string& s) { send(s.data(), s.size()); }
    void send(const char* s) { send(s, strlen(s)); }

    //数据到达时回调
    void onRead(const TcpCallBack& cb) { readcb_ = cb; };
    //当tcp缓冲区可写时回调
    void onWritable(const TcpCallBack& cb) { writablecb_ = cb;}
    //tcp状态改变时回调
    void onState(const TcpCallBack& cb) { statecb_ = cb; }
    //tcp空闲回调
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

//Tcp服务器
struct TcpServer {
    //abort if bind failed
    TcpServer(EventBase* base, Ip4Addr addr);
    TcpServer(EventBase* base, const std::string& host, short port): TcpServer(base, Ip4Addr(host, port)) {}
    TcpServer(MultiBase* bases, Ip4Addr addr):TcpServer(bases->allocBase(), addr) { bases_ = [bases] {return bases->allocBase(); }; }
    TcpServer(MultiBase* bases, const std::string& host, short port): TcpServer(bases, Ip4Addr(host, port)) {}
    ~TcpServer() { delete listen_channel_; }
    Ip4Addr getAddr() { return addr_; }
    //为每个接入的连接创建回调函数
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