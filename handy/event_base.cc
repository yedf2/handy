#include "event_base.h"
#include "logging.h"
#include "util.h"
#include <map>
#include <string.h>
#include <fcntl.h>
#include "poller.h"
#include "conn.h"
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
    IdleIdImp() {}
    typedef list<IdleNode>::iterator Iter;
    IdleIdImp(list<IdleNode>* lst, Iter iter): lst_(lst), iter_(iter){}
    list<IdleNode>* lst_;
    Iter iter_;
};

struct EventsImp {
    EventBase* base_;
    PollerBase* poller_;
    std::atomic<bool> exit_;
    int wakeupFds_[2];
    int nextTimeout_;
    SafeQueue<Task> tasks_;
    
    std::map<TimerId, TimerRepeatable> timerReps_;
    std::map<TimerId, Task> timers_;
    std::atomic<int64_t> timerSeq_;
    // 记录每个idle时间（单位秒）下所有的连接。链表中的所有连接，最新的插入到链表末尾。连接若有活动，会把连接从链表中移到链表尾部，做法参考memcache
    std::map<int, std::list<IdleNode>> idleConns_;
    std::set<TcpConnPtr> reconnectConns_;
    bool idleEnabled;

    EventsImp(EventBase* base, int taskCap);
    ~EventsImp();
    void init();
    void callIdles();
    IdleId registerIdle(int idle, const TcpConnPtr& con, const TcpCallBack& cb);
    void unregisterIdle(const IdleId& id);
    void updateIdle(const IdleId& id);
    void handleTimeouts();
    void refreshNearest(const TimerId* tid=NULL);
    void repeatableTimeout(TimerRepeatable* tr);

    //eventbase functions
    EventBase& exit() {exit_ = true; wakeup(); return *base_;}
    bool exited() { return exit_; }
    void safeCall(Task&& task) { tasks_.push(move(task)); wakeup(); }
    void loop();
    void loop_once(int waitMs) { poller_->loop_once(std::min(waitMs, nextTimeout_)); handleTimeouts(); }
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

bool EventBase::cancel(TimerId timerid) { return imp_ && imp_->cancel(timerid); }

TimerId EventBase::runAt(int64_t milli, Task&& task, int64_t interval) {
    return imp_->runAt(milli, std::move(task), interval); 
}

EventsImp::EventsImp(EventBase* base, int taskCap):
    base_(base), poller_(createPoller()), exit_(false), nextTimeout_(1<<30), tasks_(taskCap),
    timerSeq_(0), idleEnabled(false)
{
}

void EventsImp::loop() {
    while (!exit_)
        loop_once(10000);
    timerReps_.clear();
    timers_.clear();
    idleConns_.clear();
    for (auto recon: reconnectConns_) { //重连的连接无法通过channel清理，因此单独清理
        recon->cleanup(recon);
    }
    loop_once(0);
}

void EventsImp::init() {
    int r = pipe(wakeupFds_);
    fatalif(r, "pipe failed %d %s", errno, strerror(errno));
    r = util::addFdFlag(wakeupFds_[0], FD_CLOEXEC);
    fatalif(r, "addFdFlag failed %d %s", errno, strerror(errno));
    r = util::addFdFlag(wakeupFds_[1], FD_CLOEXEC);
    fatalif(r, "addFdFlag failed %d %s", errno, strerror(errno));
    trace("wakeup pipe created %d %d", wakeupFds_[0], wakeupFds_[1]);
    Channel* ch = new Channel(base_, wakeupFds_[0], kReadEvent);
    ch->onRead([=] {
        char buf[1024];
        int r = ch->fd() >= 0 ? ::read(ch->fd(), buf, sizeof buf) : 0;
        if (r > 0) {
            Task task;
            while (tasks_.pop_wait(&task, 0)) {
                task();
            }
        } else if (r == 0) {
            delete ch;
        } else if (errno == EINTR) {
        } else {
            fatal("wakeup channel read error %d %d %s", r, errno, strerror(errno));
        }
    });
}

void EventsImp::handleTimeouts() {
    int64_t now = util::timeMilli();
    TimerId tid { now, 1L<<62 };
    while (timers_.size() && timers_.begin()->first < tid) {
        Task task = move(timers_.begin()->second);
        timers_.erase(timers_.begin());
        task();
    }
    refreshNearest();
}

EventsImp::~EventsImp() {
    delete poller_;
    ::close(wakeupFds_[1]);
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
    if (timers_.empty()) {
        nextTimeout_ = 1 << 30;
    } else {
        const TimerId &t = timers_.begin()->first;
        nextTimeout_ = t.first - util::timeMilli();
        nextTimeout_ = nextTimeout_ < 0 ? 0 : nextTimeout_;
    }
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
    poller_ = base_->imp_->poller_;
    poller_->addChannel(this);
}

Channel::~Channel() {
    close();
}

void Channel::enableRead(bool enable) {
    if (enable) {
        events_ |= kReadEvent;
    } else {
        events_ &= ~kReadEvent;
    }
    poller_->updateChannel(this);
}

void Channel::enableWrite(bool enable) {
    if (enable) {
        events_ |= kWriteEvent;
    } else {
        events_ &= ~kWriteEvent;
    }
    poller_->updateChannel(this);
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
    poller_->updateChannel(this);
}

void Channel::close() {
    if (fd_>=0) {
        trace("close channel %ld fd %d", (long)id_, fd_);
        poller_->removeChannel(this);
        ::close(fd_);
        fd_ = -1;
        handleRead();
    }
}

bool Channel::readEnabled() { return events_ & kReadEvent; }
bool Channel::writeEnabled() { return events_ & kWriteEvent; }

void handyUnregisterIdle(EventBase* base, const IdleId& idle) {
    base->imp_->unregisterIdle(idle);
}

void handyUpdateIdle(EventBase* base, const IdleId& idle) {
    base->imp_->updateIdle(idle);
}

TcpConn::TcpConn()
:base_(NULL), channel_(NULL), state_(State::Invalid), destPort_(-1),
 connectTimeout_(0), reconnectInterval_(-1),connectedTime_(util::timeMilli())
{
}

TcpConn::~TcpConn() {
    trace("tcp destroyed %s - %s", local_.toString().c_str(), peer_.toString().c_str());
    delete channel_;
}

void TcpConn::addIdleCB(int idle, const TcpCallBack& cb) {
    if (channel_) {
        idleIds_.push_back(getBase()->imp_->registerIdle(idle, shared_from_this(), cb));
    }
}

void TcpConn::reconnect() {
    auto con = shared_from_this();
    getBase()->imp_->reconnectConns_.insert(con);
    long long interval = reconnectInterval_-(util::timeMilli()-connectedTime_);
    interval = interval>0?interval:0;
    info("reconnect interval: %d will reconnect after %lld ms", reconnectInterval_, interval);
    getBase()->runAfter(interval, [this, con]() {
        getBase()->imp_->reconnectConns_.erase(con);
        connect(getBase(), destHost_, (short)destPort_, connectTimeout_, localIp_);
    });
    delete channel_;
    channel_ = NULL;
}

}