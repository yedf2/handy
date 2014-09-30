#include "handy.h"
#include "logging.h"
#include "util.h"
#include <map>
#include <string.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <fcntl.h>

using namespace std;

namespace handy {

namespace {

const int kReadEvent = EPOLLIN;
const int kWriteEvent = EPOLLOUT;
const int kMaxEvents = 20;

struct TimerRepeatable {
    int64_t at; //current timer timeout timestamp
    int64_t interval;
    TimerId timerid;
    Task cb;
};

struct IdleNode {
    TcpConnPtr con_;
    int64_t updated_;
    TcpCallBack cb_;
};

}

struct IdleIdImp {
    IdleIdImp() {}
    typedef list<IdleNode>::iterator Iter;
    IdleIdImp(list<IdleNode>* lst, Iter iter): lst_(lst), iter_(iter){}
    list<IdleNode>* lst_;
    Iter iter_;
};

struct EventsImp {
    EventBase* base_;

    std::atomic<bool> exit_;
    std::list<Channel*> liveChannels_;
    int wakeupFds_[2];
    int epollfd_;
    SafeQueue<Task> tasks_;
    
    //for epoll selected active events
    struct epoll_event activeEvs_[kMaxEvents];
    int lastActive_;

    std::map<TimerId, TimerRepeatable> timerReps_;
    std::map<TimerId, Task> timers_;
    std::atomic<int64_t> timerSeq_;
    int timerfd_;
    std::map<int, std::list<IdleNode>> idleConns_;
    bool idleEnabled;

    EventsImp(EventBase* base, int taskCap);
    ~EventsImp();
    void init();
    void addChannel(Channel* ch);
    void removeChannel(Channel* ch);
    void updateChannel(Channel* ch);

    void callIdles();
    IdleId registerIdle(int idle, const TcpConnPtr& con, const TcpCallBack& cb);
    void unregisterIdle(const IdleId& id);
    void updateIdle(const IdleId& id);
    void refreshNearest(const TimerId* tid=NULL);
    void repeatableTimeout(TimerRepeatable* tr);

    //eventbase functions
    EventBase& exit() { exit_ = true; wakeup(); return *base_; }
    bool exited() { return exit_; }
    void safeCall(Task&& task) { tasks_.push(move(task)); wakeup(); }
    void loop() { while (!exit_) loop_once(10000); loop_once(0); }
    void loop_once(int waitMs);
    void wakeup() {
        int r = write(wakeupFds_[1], "", 1);
        fatalif(r<=0, "write error wd %d %d %s", r, errno, strerror(errno));
    }

    bool cancel(TimerId timerid);
    TimerId runAt(int64_t milli, Task&& task, int64_t interval);
};

EventBase::EventBase(int taskCapacity) {
    imp_.reset(new EventsImp(this, taskCapacity));
    imp_->init();
}

EventBase::~EventBase() {}

EventBase& EventBase::exit() { return imp_->exit(); }

bool EventBase::exited() { return imp_->exited(); }

void EventBase::safeCall(Task&& task) { imp_->safeCall(move(task)); }

void EventBase::wakeup(){ imp_->wakeup(); }

void EventBase::loop() { imp_->loop(); }

void EventBase::loop_once(int waitMs) { imp_->loop_once(waitMs); }

bool EventBase::cancel(TimerId timerid) { return imp_->cancel(timerid); }

TimerId EventBase::runAt(int64_t milli, Task&& task, int64_t interval) {
    return imp_->runAt(milli, std::move(task), interval); 
}

EventsImp::EventsImp(EventBase* base, int taskCap):
    base_(base), exit_(false), tasks_(taskCap), lastActive_(-1),
    timerSeq_(0), idleEnabled(false)
{
}

void EventsImp::init() {
    epollfd_ = epoll_create1(EPOLL_CLOEXEC);
    fatalif(epollfd_<0, "epoll_create error %d %s", errno, strerror(errno));
    info("event base %d created", epollfd_);
    int r = pipe2(wakeupFds_, O_CLOEXEC);
    fatalif(r, "pipe failed %d %s", errno, strerror(errno));
    trace("wakeup pipe created %d %d", wakeupFds_[0], wakeupFds_[1]);
    Channel* ch = new Channel(base_, wakeupFds_[0], kReadEvent);
    ch->onRead([=] {
        char buf[1024];
        int r = ::read(ch->fd(), buf, sizeof buf);
        if (r >= 0) {
            Task task;
            while (tasks_.pop_wait(&task, 0)) {
                task();
            }
        } else if (errno == EBADF) {
            delete ch;
        } else if (errno == EINTR) {
        } else {
            fatal("wakeup channel read error %d %d %s", r, errno, strerror(errno));
        }
    });

    timerfd_ = ::timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
    fatalif (timerfd_ < 0, "timerfd create failed %d %s", errno, strerror(errno));
    trace("timerfd %d created", timerfd_);
    Channel* timer = new Channel(base_, timerfd_, EPOLLIN);
    timer->onRead([=] {
        char buf[256];
        int r = ::read(timerfd_, buf, sizeof buf);
        if (r < 0 && errno == EBADF) {
            delete timer;
            return;
        } else if (r < 0 && errno == EINTR) {
            return;
        } else if (r == 0) { //closed
            return;
        }
        int64_t now = util::timeMilli();
        TimerId tid { now, 1L<<62 };
        while (timers_.size() && timers_.begin()->first < tid) {
            Task task = move(timers_.begin()->second);
            timers_.erase(timers_.begin());
            task();
        }
        if (timers_.size()) {
            refreshNearest();
        }
    });
}

EventsImp::~EventsImp() {
    info("destroying event base %d", epollfd_);
    for (auto ch: liveChannels_) {
        ch->close();
    }
    while (liveChannels_.size()) {
        (*liveChannels_.begin())->handleRead();
    }
    ::close(wakeupFds_[1]);
    ::close(epollfd_);
    info("event base %d destroyed", epollfd_);
}

void EventsImp::addChannel(Channel* ch) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = ch->events();
    ev.data.ptr = ch;
    trace("adding channel %ld fd %d events %d epoll %d", ch->id(), ch->fd(), ev.events, epollfd_);
    int r = epoll_ctl(epollfd_, EPOLL_CTL_ADD, ch->fd(), &ev);
    fatalif(r, "epoll_ctl add failed %d %s", errno, strerror(errno));
    liveChannels_.push_front(ch);
    ch->eventPos_ = liveChannels_.begin();
}

void EventsImp::updateChannel(Channel* ch) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = ch->events();
    ev.data.ptr = ch;
    trace("modifying channel %ld fd %d events %d epoll %d", ch->id(), ch->fd(), ev.events, epollfd_);
    int r = epoll_ctl(epollfd_, EPOLL_CTL_MOD, ch->fd(), &ev);
    fatalif(r, "epoll_ctl mod failed %d %s", errno, strerror(errno));
}

void EventsImp::removeChannel(Channel* ch) {
    trace("deleting channel %ld fd %d epoll %d", ch->id(), ch->fd(), epollfd_);
    int r = epoll_ctl(epollfd_, EPOLL_CTL_DEL, ch->fd(), NULL);
    fatalif(r && errno != EBADF, "epoll_ctl fd: %d del failed %d %s", ch->fd(), errno, strerror(errno));
    liveChannels_.erase(ch->eventPos_);
    for (int i = lastActive_; i >= 0; i --) {
        if (ch == activeEvs_[i].data.ptr) {
            activeEvs_[i].data.ptr = NULL;
            break;
        }
    }
}

void EventsImp::loop_once(int waitMs) {
    lastActive_ = epoll_wait(epollfd_, activeEvs_, kMaxEvents, waitMs);
    while (--lastActive_ >= 0) {
        int i = lastActive_;
        Channel* ch = (Channel*)activeEvs_[i].data.ptr;
        int events = activeEvs_[i].events;
        if (ch) {
            if (events & (kReadEvent | EPOLLERR)) {
                trace("channel %ld fd %d handle read", ch->id(), ch->fd());
                ch->handleRead();
            } else if (events & kWriteEvent) {
                trace("channel %ld fd %d handle write", ch->id(), ch->fd());
                ch->handleWrite();
            } else {
                fatal("unexpected epoll events");
            }
        }
    }
}

void EventsImp::callIdles() {
    int64_t now = util::timeMilli() / 1000;
    for (auto& l: idleConns_) {
        int idle = l.first;
        auto lst = l.second;
        while(lst.size()) {
            IdleNode& node = lst.front();
            if (node.updated_ + idle > now) {
                break;
            }
            node.updated_ = now;
            lst.splice(lst.end(), lst, lst.begin());
            node.cb_(node.con_);
        }
    }
}

IdleId EventsImp::registerIdle(int idle, const TcpConnPtr& con, const TcpCallBack& cb) {
    if (!idleEnabled) {
        base_->runAfter(1000, [this] { callIdles(); }, 1000);
        idleEnabled = true;
    }
    auto& lst = idleConns_[idle];
    lst.push_back(IdleNode {con, util::timeMilli()/1000, move(cb) });
    trace("register idle");
    return IdleId(new IdleIdImp(&lst, --lst.end()));
}

void EventsImp::unregisterIdle(const IdleId& id) {
    trace("unregister idle");
    id->lst_->erase(id->iter_);
}

void EventsImp::updateIdle(const IdleId& id) {
    trace("update idle");
    id->iter_->updated_ = util::timeMilli() / 1000;
    id->lst_->splice(id->lst_->end(), *id->lst_, id->iter_);
}

void EventsImp::refreshNearest(const TimerId* tid){
    const TimerId& t = timers_.begin()->first;
    if (tid && tid->first != t.first) { //newly added tid is not nearest
        return;
    }
    struct itimerspec spec;
    memset(&spec, 0, sizeof spec);
    int64_t next = t.first - util::timeMilli();
    next = max(1L, next); //in case next is negative or zero
    spec.it_value.tv_sec = next / 1000;
    spec.it_value.tv_nsec = (next % 1000) * 1000 * 1000;
    int r = ::timerfd_settime(timerfd_, 0, &spec, NULL);
    fatalif(r, "timerfd_settime error. %d %d %s", r, errno, strerror(errno));
}

void EventsImp::repeatableTimeout(TimerRepeatable* tr) {
    tr->at += tr->interval;
    tr->timerid = { tr->at, ++timerSeq_ };
    timers_[tr->timerid] = [this, tr] { repeatableTimeout(tr); };
    refreshNearest(&tr->timerid);
    tr->cb();
}

TimerId EventsImp::runAt(int64_t milli, Task&& task, int64_t interval) {
    if (exit_) {
        return TimerId();
    }
    if (interval) {
        TimerId tid { -milli, ++timerSeq_};
        TimerRepeatable& rtr = timerReps_[tid];
        rtr = { milli, interval, { milli, ++timerSeq_ }, move(task) };
        TimerRepeatable* tr = &rtr;
        timers_[tr->timerid] = [this, tr] { repeatableTimeout(tr); };
        refreshNearest(&tr->timerid);
        return tid;
    } else {
        TimerId tid { milli, ++timerSeq_};
        timers_.insert({tid, move(task)});
        refreshNearest(&tid);
        return tid;
    }
}

bool EventsImp::cancel(TimerId timerid) {
    if (timerid.first < 0) {
        auto p = timerReps_.find(timerid);
        auto ptimer = timers_.find(p->second.timerid);
        if (ptimer != timers_.end()) {
            timers_.erase(ptimer);
        }
        timerReps_.erase(p);
        return true;
    } else {
        auto p = timers_.find(timerid);
        if (p != timers_.end()) {
            timers_.erase(p);
            return true;
        }
        return false;
    }
}

void MultiBase::loop() {
    int sz = bases_.size();
    vector<thread> ths(sz -1);
    for(int i = 0; i < sz -1; i++) {
        thread t([this, i]{ bases_[i].loop();});
        ths[i].swap(t);
    }
    bases_.back().loop();
    for (int i = 0; i < sz -1; i++) {
        ths[i].join();
    }
}

Channel::Channel(EventBase* base, int fd, int events): base_(base), fd_(fd), events_(events) {
    fatalif(net::setNonBlock(fd_) < 0, "channel set non block failed");
    static atomic<int64_t> id(0);
    id_ = ++id;
    base_->imp_->addChannel(this);
}

Channel::~Channel() { 
    base_->imp_->removeChannel(this);
    if (fd_>=0) {
        ::close(fd_);
    }
}

void Channel::enableRead(bool enable) {
    if (enable) {
        events_ |= kReadEvent;
    } else {
        events_ &= ~kReadEvent;
    }
    base_->imp_->updateChannel(this);
}

void Channel::enableWrite(bool enable) {
    if (enable) {
        events_ |= kWriteEvent;
    } else {
        events_ &= kWriteEvent;
    }
    base_->imp_->updateChannel(this);
}

void Channel::enableReadWrite(bool readable, bool writable) {
    if (readable) {
        events_ |= kReadEvent;
    } else {
        events_ &= ~kReadEvent;
    }
    if (writable) {
        events_ |= kWriteEvent;
    } else {
        events_ &= ~kWriteEvent;
    }
    base_->imp_->updateChannel(this);
}

void Channel::close() {
    if (fd_ >= 0) {
        trace("close channel %ld fd %d", id_, fd_);
        ::close(fd_);
        fd_ = -1;
    }
}

bool Channel::readEnabled() { return events_ & kReadEvent; }
bool Channel::writeEnabled() { return events_ & kWriteEvent; }

TcpConn::TcpConn(EventBase* base, int fd, Ip4Addr local, Ip4Addr peer)
        :local_(local), peer_(peer), state_(State::Connecting)
{
    channel_ = new Channel(base, fd, EPOLLOUT);
    trace("tcp constructed %s - %s fd: %d",
        local_.toString().c_str(),
        peer_.toString().c_str(),
        fd);
}

TcpConnPtr TcpConn::create(EventBase* base, int fd, Ip4Addr local, Ip4Addr peer)
{
    TcpConnPtr con(new TcpConn(base, fd, local, peer));
    con->channel_->onRead([=] { con->handleRead(con); });
    con->channel_->onWrite([=] { con->handleWrite(con); });
    return con;
}

TcpConnPtr TcpConn::connectTo(EventBase* base, Ip4Addr addr, int timeout) {
    int fd = socket(AF_INET, SOCK_STREAM|SOCK_CLOEXEC, 0);
    net::setNonBlock(fd);
    int r = ::connect(fd, (sockaddr*)&addr.getAddr(), sizeof (sockaddr_in));
    if (r != 0 && errno != EINPROGRESS) {
        error("connect to %s error %d %s", addr.toString().c_str(), errno, strerror(errno));
        ::close(fd);
        return NULL;
    }

    sockaddr_in local;
    socklen_t alen = sizeof(local);
    r = getsockname(fd, (sockaddr*)&local, &alen);
    if (r < 0) {
        error("getsockname failed %d %s", errno, strerror(errno));
        ::close(fd);
        return NULL;
    }
    if (timeout) {
        TcpConnPtr con = TcpConn::create(base, fd, Ip4Addr(local), addr);
        base->runAfter(timeout, [con] {
            if (con->getState() == Connecting) { con->close(true); }
        });
        return con;
    }
    return TcpConn::create(base, fd, Ip4Addr(local), addr);

}

TcpConn::~TcpConn() {
    trace("tcp destroyed %s - %s", local_.toString().c_str(), peer_.toString().c_str());
    delete channel_;
}

void TcpConn::close(bool cleanupNow) {
    if (channel_) {
        channel_->close();
        if (cleanupNow) {
            channel_->handleRead();
        } else {
            TcpConnPtr con = shared_from_this();
            getBase()->safeCall([con]{ if (con->channel_) con->channel_->handleRead(); });
        }
    }
}

void TcpConn::handleRead(const TcpConnPtr& con) {
    for(;;) {
        input_.makeRoom();
        int rd = 0;
        if (channel_->fd() >= 0) {
            rd = ::read(channel_->fd(), input_.end(), input_.space());
            trace("channel %ld fd %d readed %d bytes", channel_->id(), channel_->fd(), rd);
        }
        if (rd == -1 && errno == EINTR) {
            continue;
        } else if (rd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK) ) {
            for(auto& idle: idleIds_) {
                getBase()->imp_->updateIdle(idle);
            }
            if (readcb_) {
                readcb_(con);
            }
            break;
        } else if (channel_->fd() == -1 || rd == 0 || rd == -1) {
            if (state_ == State::Connecting) {
                state_ = State::Failed;
            } else {
                state_ = State::Closed;
            }
            trace("tcp closing %s - %s fd %d %d",
                local_.toString().c_str(),
                peer_.toString().c_str(),
                channel_->fd(), errno);
            for (auto& idle: idleIds_) {
                getBase()->imp_->unregisterIdle(idle);
            }
            if (statecb_) {
                statecb_(con);
            }
            //channel may have hold TcpConnPtr, set channel_ to NULL before delete
            Channel* ch = channel_;
            channel_ = NULL;
            readcb_ = writablecb_ = statecb_ = nullptr;
            delete ch;
            break;
        } else { //rd > 0
            input_.addSize(rd);
        }
    }
}

void TcpConn::handleWrite(const TcpConnPtr& con) {
    if (state_ == State::Connecting) {
        trace("tcp connected %s - %s fd %d",
            local_.toString().c_str(),
            peer_.toString().c_str(),
            channel_->fd());
        state_ = State::Connected;
        channel_->enableReadWrite(true, false);
        if (statecb_) {
            statecb_(con);
        }
    } else if (state_ == State::Connected) {
        ssize_t sended = isend(output_.begin(), output_.size());
        output_.consume(sended);
        if (output_.empty() && writablecb_) {
            writablecb_(con);
        }
        if (output_.empty()) { // writablecb_ may write something
            channel_->enableWrite(false);
        }
    } else {
        error("handle write unexpected");
    }
}

void TcpConn::addIdleCB(int idle, const TcpCallBack& cb) {
    if (channel_) {
        idleIds_.push_back(getBase()->imp_->registerIdle(idle, shared_from_this(), std::move(cb)) );
    }
}

ssize_t TcpConn::isend(const char* buf, size_t len) {
    size_t sended = 0;
    while (len > sended) {
        ssize_t wd = ::write(channel_->fd(), buf + sended, len - sended);
        trace("channel %ld fd %d write %ld bytes", channel_->id(), channel_->fd(), wd);
        if (wd > 0) {
            sended += wd;
            continue;
        } else if (wd == -1 && errno == EINTR) {
            continue;
        } else if (wd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            channel_->enableWrite(true);
            break;
        } else {
            error("write error: wd %ld %d %s", wd, errno, strerror(errno));
            break;
        }
    }
    return sended;
}

void TcpConn::send(Buffer& buf) {
    if (channel_) {
        if (channel_->writeEnabled()) { //just full
            output_.absorb(buf);
        } 
        if (buf.size()) {
            ssize_t sended = isend(buf.begin(), buf.size());
            buf.consume(sended);
        }
        if (buf.size()) {
            output_.absorb(buf);
            if (!channel_->writeEnabled()) {
                channel_->enableWrite(true);
            }
        }
    } else {
        warn("connection %s - %s closed, but still writing %lu bytes",
            local_.toString().c_str(), peer_.toString().c_str(), buf.size());
    }
}

void TcpConn::send(const char* buf, size_t len) {
    if (channel_) {
        if (output_.empty()) {
            ssize_t sended = isend(buf, len);
            buf += sended;
            len -= sended;
        }
        if (len) {
            output_.append(buf, len);
        }
    } else {
        warn("connection %s - %s closed, but still writing %lu bytes",
            local_.toString().c_str(), peer_.toString().c_str(), len);
    }
}

TcpServer::TcpServer(EventBase* base, Ip4Addr addr): base_(base), addr_(addr) {
    int fd = socket(AF_INET, SOCK_STREAM|SOCK_CLOEXEC, 0);
    int r = net::setReuseAddr(fd);
    fatalif(r, "set socket reuse option failed");
    r = ::bind(fd,(struct sockaddr *)&addr_.getAddr(),sizeof(struct sockaddr));
    fatalif(r, "bind to %s failed %d %s", addr_.toString().c_str(), errno, strerror(errno));
    r = listen(fd, 20);
    fatalif(r, "listen failed %d %s", errno, strerror(errno));
    info("fd %d listening at %s", fd, addr_.toString().c_str());
    listen_channel_ = new Channel(base, fd, EPOLLIN);
    listen_channel_->onRead(bind(&TcpServer::handleAccept, this));
}

void TcpServer::handleAccept() {
    struct sockaddr_in raddr;
    socklen_t rsz = sizeof(raddr);
    int cfd;
    while ((cfd = accept4(listen_channel_->fd(),(struct sockaddr *)&raddr,&rsz, SOCK_CLOEXEC))>=0) {
        sockaddr_in peer, local;
        socklen_t alen = sizeof(peer);
        int r = getpeername(cfd, (sockaddr*)&peer, &alen);
        if (r < 0) {
            error("get peer name failed %d %s", errno, strerror(errno));
            continue;
        }
        r = getsockname(cfd, (sockaddr*)&local, &alen);
        if (r < 0) {
            error("getsockname failed %d %s", errno, strerror(errno));
            continue;
        }
        EventBase* b = base_;
        if (bases_) {
            b = bases_();
        }
        auto addcon = [=] {
            TcpConnPtr con =TcpConn::create(b, cfd, local, peer);
            con->onRead(readcb_);
            con->onWritable(writablecb_);
            con->onState(statecb_);
            for(auto& idle: idles_) {
                con->addIdleCB(idle.first, idle.second);
            }
        };
        if (b == base_) {
            addcon();
        } else {
            b->safeCall(move(addcon));
        }
    }
    if (errno != EAGAIN && errno != EINTR) {
        warn("accept return %d  %d %s", cfd, errno, strerror(errno));
    }
}

}