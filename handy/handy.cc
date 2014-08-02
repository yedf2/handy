#include "handy.h"
#include "logging.h"
#include "util.h"
#include <map>
#include <string.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>

using namespace std;

namespace handy {
namespace {
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
    IdleIdImp(): idle_(0) {}
    typedef list<IdleNode>::iterator Iter;
    IdleIdImp(list<IdleNode>* lst, Iter iter, int idle): lst_(lst), iter_(iter), idle_(idle){}
    list<IdleNode>* lst_;
    Iter iter_;
    int idle_;
};

struct TimerImp {
    std::map<TimerId, TimerRepeatable> timerReps_;
    std::map<TimerId, Task> timers_;
    std::atomic<int64_t> timerSeq_;
    int timerfd_;
    std::map<int, std::list<IdleNode>> idleConns_;
    void callIdles() {
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
    IdleId registerIdle(int idle, const TcpConnPtr& con, TcpCallBack&& cb) {
        auto& lst = idleConns_[idle];
        lst.push_back(IdleNode {con, util::timeMilli()/1000, move(cb) });
        debug("register idle");
        return IdleId(new IdleIdImp(&lst, --lst.end(), idle));
    }

    void unregisterIdle(const IdleId& id) {
        if (id) {
            debug("unregister idle");
            id->lst_->erase(id->iter_);
        }
    }

    void updateIdle(const IdleId& id) {
        if (id) {
            debug("update idle");
            id->iter_->updated_ = util::timeMilli() / 1000;
            id->lst_->splice(id->lst_->end(), *id->lst_, id->iter_);
        }
    }

    void refreshNearest(const TimerId* tid=NULL){
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

    void repeatableTimeout(TimerRepeatable* tr) {
        tr->at += tr->interval;
        tr->timerid = { tr->at, ++timerSeq_ };
        timers_[tr->timerid] = [this, tr] { repeatableTimeout(tr); };
        refreshNearest(&tr->timerid);
        tr->cb();
    }
    TimerId runAt(int64_t milli, Task&& task, int64_t interval) {
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

    bool cancel(TimerId timerid) {
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

};

const int EventBase::kReadEvent = EPOLLIN;
const int EventBase::kWriteEvent = EPOLLOUT;

EventBase::EventBase(int maxTasks):exit_(false), tasks_(maxTasks) {
    epollfd = epoll_create(10);
    fatalif(epollfd<0, "epoll_create error %d %s", errno, strerror(errno));
    info("event base %d created", epollfd);
    int r = pipe(wakeupFds_);
    fatalif(r, "pipe failed %d %s", errno, strerror(errno));
    Channel* ch = new Channel(this, wakeupFds_[0], EPOLLIN);
    EventBase* pth = this;
    ch->onRead([=] {
        char buf[1024];
        int r = ::read(ch->fd(), buf, sizeof buf);
        if (r >= 0) {
            Task task;
            if (pth->tasks_.pop_wait(&task, 0)) {
                task();
            }
        } else if (errno == EBADF) {
            delete ch;
        } else if (errno == EINTR) {
        } else {
            fatal("wakeup channel read error %d %d %s", r, errno, strerror(errno));
        }
    });
    timerImp_.reset(new TimerImp);
    timerImp_->timerSeq_ = 0;
    timerImp_->timerfd_ = ::timerfd_create(CLOCK_MONOTONIC, 0);
    fatalif (timerImp_->timerfd_ < 0, "timerfd create failed %d %s", errno, strerror(errno));
    Channel* timer = new Channel(this, timerImp_->timerfd_, EPOLLIN);
    timer->onRead([=] {
        char buf[256];
        int r = ::read(timerImp_->timerfd_, buf, sizeof buf);
        if (r < 0 && errno == EBADF) {
            delete timer;
            return;
        } else if (r < 0 && errno == EINTR) {
            return;
        }
        int64_t now = util::timeMilli();
        TimerId tid { now, 1L<<62 };
        map<TimerId, Task>& timers_ = timerImp_->timers_;
        while (timers_.size() && timers_.begin()->first < tid) {
            Task task = move(timers_.begin()->second);
            timers_.erase(timers_.begin());
            task();
        }
        if (timers_.size()) {
            timerImp_->refreshNearest();
        }
    });
    runAfter(1000, [this] { timerImp_->callIdles(); }, 1000);
}

EventBase::~EventBase() {
    info("destroying event base %d", epollfd);
    ::close(wakeupFds_[1]);
    for (auto ch: live_channels_) {
        ::close(ch->fd());
    }
    exit_ = true;
    while (live_channels_.size()) {
        (*live_channels_.begin())->handleRead();
    }
    close(epollfd);
    info("event base %d destroyed", epollfd);
}

void EventBase::wakeup(){ 
    int r = ::write(wakeupFds_[1], "", 1); 
    fatalif(r<=0, "write error wd %d %d %s", r, errno, strerror(errno)); 
}

void EventBase::addChannel(Channel* ch) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = ch->events();
    ev.data.ptr = ch;
    debug("adding fd %d events %d epoll %d", ch->fd(), ev.events, epollfd);
    int r = epoll_ctl(epollfd, EPOLL_CTL_ADD, ch->fd(), &ev);
    fatalif(r, "epoll_ctl add failed %d %s", errno, strerror(errno));
    live_channels_.insert(ch);
}

void EventBase::updateChannel(Channel* ch) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = ch->events();
    ev.data.ptr = ch;
    debug("modifying fd %d events %d epoll %d", ch->fd(), ev.events, epollfd);
    int r = epoll_ctl(epollfd, EPOLL_CTL_MOD, ch->fd(), &ev);
    fatalif(r, "epoll_ctl mod failed %d %s", errno, strerror(errno));
}

void EventBase::removeChannel(Channel* ch) {
    debug("deleting fd %d epoll %d", ch->fd(), epollfd);
    live_channels_.erase(ch);
    int r = epoll_ctl(epollfd, EPOLL_CTL_DEL, ch->fd(), NULL);
    fatalif(r && errno != EBADF, "epoll_ctl fd: %d del failed %d %s", ch->fd(), errno, strerror(errno));
}

void EventBase::loop_once(int waitMs) {
    struct epoll_event evs[kMaxEvents];
    int n = epoll_wait(epollfd, evs, kMaxEvents, waitMs);
    for (int i = 0; i < n; i ++) {
        Channel* ch = (Channel*)evs[i].data.ptr;
        //handleEvent may delete some channel, which we may hold pointer
        if (live_channels_.find(ch) != live_channels_.end()) {
            int events = evs[i].events;
            if (events & (kReadEvent | EPOLLERR)) {
                debug("handle read");
                ch->handleRead();
            } else if (events & kWriteEvent) {
                debug("handle write");
                ch->handleWrite();
            } else {
                fatal("unexpected epoll events");
            }
        }
    }
}

bool EventBase::cancel(TimerId timerid) { 
    return timerImp_->cancel(timerid); 
}

TimerId EventBase::runAt(int64_t milli, Task&& task, int64_t interval) { 
    return timerImp_->runAt(milli, std::move(task), interval); 
}

Channel::Channel(EventBase* base, int fd, int events): base_(base), fd_(fd), events_(events) {
    fatalif(net::setNonBlock(fd_) < 0, "channel set non block failed");
    base_->addChannel(this);
    readcb_ = writecb_ = []{};
}

void Channel::enableRead(bool enable) {
    if (enable) {
        events_ |= EventBase::kReadEvent;
    } else {
        events_ &= ~EventBase::kReadEvent;
    }
    base_->updateChannel(this);
}

void Channel::enableWrite(bool enable) {
    if (enable) {
        events_ |= EventBase::kWriteEvent;
    } else {
        events_ &= ~EventBase::kWriteEvent;
    }
    base_->updateChannel(this);
}

void Channel::enableReadWrite(bool readable, bool writable) {
    if (readable) {
        events_ |= EventBase::kReadEvent;
    } else {
        events_ &= ~EventBase::kReadEvent;
    }
    if (writable) {
        events_ |= EventBase::kWriteEvent;
    } else {
        events_ &= ~EventBase::kWriteEvent;
    }
    base_->updateChannel(this);
}

TcpConn::TcpConn(EventBase* base, int fd, Ip4Addr local, Ip4Addr peer)
        :local_(local), peer_(peer), state_(State::Connecting)
{
    channel_ = new Channel(base, fd, EPOLLOUT);
    readcb_ = writablecb_ = statecb_ = [](const TcpConnPtr&) {};
    debug("tcp constructed %s - %s fd: %d",
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

TcpConnPtr TcpConn::connectTo(EventBase* base, Ip4Addr addr) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
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
    return TcpConn::create(base, fd, Ip4Addr(local), addr);

}

TcpConn::~TcpConn() {
    debug("tcp destroyed %s - %s", local_.toString().c_str(), peer_.toString().c_str());
    delete channel_;
}

void TcpConn::close() {
    if (channel_) {
        ::close(channel_->fd());
        TcpConnPtr con = shared_from_this();
        getBase()->safeCall([con]{ if (con->channel_) con->channel_->handleRead(); });
    }
}

void TcpConn::handleRead(const TcpConnPtr& con) {
    for(;;) {
        input_.makeRoom();
        int rd = ::read(channel_->fd(), input_.end(), input_.space());
        debug("fd %d readed %d bytes", channel_->fd(), rd);
        if (rd == -1 && errno == EINTR) {
            continue;
        } else if (rd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK) ) {
            getBase()->timerImp_->updateIdle(idleId_);
            readcb_(con);
            break;
        } else if (rd == 0 || rd == -1) {
            if (state_ == State::Connecting) {
                state_ = State::Failed;
            } else {
                state_ = State::Closed;
            }
            debug("tcp closing %s - %s fd %d",
                local_.toString().c_str(),
                peer_.toString().c_str(),
                channel_->fd());
            getBase()->timerImp_->unregisterIdle(idleId_);
            statecb_(con);
            //channel may have hold TcpConnPtr, set channel_ to NULL before delete
            Channel* ch = channel_;
            channel_ = NULL;
            readcb_ = writablecb_ = statecb_ = [](const TcpConnPtr&) {};
            delete ch;
            break;
        } else { //rd > 0
            input_.addSize(rd);
        }
    }
}

void TcpConn::handleWrite(const TcpConnPtr& con) {
    if (state_ == State::Connecting) {
        debug("tcp connected %s - %s fd %d",
            local_.toString().c_str(),
            peer_.toString().c_str(),
            channel_->fd());
        state_ = State::Connected;
        channel_->enableReadWrite(true, false);
        statecb_(con);
    } else if (state_ == State::Connected) {
        ssize_t sended = isend(output_.begin(), output_.size());
        output_.consume(sended);
        if (output_.empty()) {
            writablecb_(con);
        }
        if (output_.empty()) { // writablecb_ may write something
            channel_->enableWrite(false);
        }
    } else {
        error("handle write unexpected");
    }
}

void TcpConn::onIdle(int idle, TcpCallBack&& cb) {
    if (channel_) {
        getBase()->timerImp_->unregisterIdle(idleId_);
    }
    idleId_ = IdleId();
    if (channel_) {
        idleId_ = getBase()->timerImp_->registerIdle(idle, shared_from_this(), std::move(cb));
    }
}

ssize_t TcpConn::isend(const char* buf, size_t len) {
    size_t sended = 0;
    while (len > sended) {
        ssize_t wd = ::write(channel_->fd(), buf + sended, len - sended);
        debug("fd %d write %ld bytes", channel_->fd(), wd);
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

TcpServer::TcpServer(EventBase* base, Ip4Addr addr): base_(base), addr_(addr), idle_(0) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int r = net::setReuseAddr(fd);
    fatalif(r, "set socket reuse option failed");
    r = ::bind(fd,(struct sockaddr *)&addr_.getAddr(),sizeof(struct sockaddr));
    fatalif(r, "bind to %s failed %d %s", addr_.toString().c_str(), errno, strerror(errno));
    r = listen(fd, 20);
    fatalif(r, "listen failed %d %s", errno, strerror(errno));
    info("fd %d listening at %s", fd, addr_.toString().c_str());
    listen_channel_ = new Channel(base, fd, EPOLLIN);
    listen_channel_->onRead(bind(&TcpServer::handleAccept, this));
    readcb_ = writablecb_ = statecb_ = idlecb_ = [](const TcpConnPtr&) {};
}

void TcpServer::handleAccept() {
    struct sockaddr_in raddr;
    socklen_t rsz = sizeof(raddr);
    int cfd;
    while ((cfd = accept(listen_channel_->fd(),(struct sockaddr *)&raddr,&rsz))>=0) {
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
        TcpConnPtr con =TcpConn::create(base_, cfd, local, peer);
        con->onRead(readcb_);
        con->onWritable(writablecb_);
        con->onState(statecb_);
        if (idle_) {
            con->onIdle(idle_, idlecb_);
        }
    }
    if (errno != EAGAIN && errno != EINTR) {
        warn("accept return %d  %d %s", cfd, errno, strerror(errno));
    }
}

}