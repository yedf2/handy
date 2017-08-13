# handy
yedongfu

A C++11 non-blocking network library

[Example](#sample)  
[EventBase events dispatcher](#event-base)  
[tcp connection](#tcp-conn)  
[tcp server](#tcp-server)  
[http server](#http-server)  
[half sync half async server](#hsha)  
<h2 id="sample">example--echo</h2>

```c
#include <handy/handy.h>

using namespace std;
using namespace handy;


int main(int argc, const char* argv[]) {
    EventBase base;
    //handle ctrl+c
    Signal::signal(SIGINT, [&]{ base.exit(); }); 
    TcpServer echo(&base);
    int r = echo.bind("", 2099);
    exitif(r, "bind failed %d %s", errno, strerror(errno));
    echo.onConnRead([](const TcpConnPtr& con) {
        con->send(con->getInput());
    });
    base.loop(); //enter events loop
}
```
<h2 id="event-base">EventBase: events dispatcher</h2>
EventBase is an events dispatcher，use epoll/kqueue to handle non-blocking I/O

```c
EventBase base;
```

### events loop

```c
//call epoll_wait repeatedly, handle I/O events
base.loop();
```

### exit events loop

```c
//exit events loop, can be called from other threads
base.exit();
```

### is events loop exited?

```c
bool exited();
```

### add tcp connection

```c
TcpConnPtr con = TcpConn::createConnection(&base, host, port);
```

### add listen fd, and will add all socket return by accept(listen_fd)

```c
TcpServer echo(&base);
```

### perform tasks in I/O thread
Some tasks must be called from I/O thread, for example writing some data to connection.
In order to avoid conflicting read/write, the operation should be performed in a single thread.

```c
void safeCall(const Task& task);

base.safeCall([](con){con->send("OK");});
```

### manage timeout tasks
EventBase will make itself return by setting a wait time form epoll_wait/kevent.
It will check and call timeout tasks. The precision rely on epoll_wait/kevent

```c
//interval: 0：once task；>0：repeated task, task will be execute every interval milliseconds
TimerId runAfter(int64_t milli, const Task& task, int64_t interval=0);
//runAt will specify the absolute time
TimerId runAt(int64_t milli, const Task& task, int64_t interval=0)
//cancel Task, Ignore if task is already removed or expired.
bool cancel(TimerId timerid);

TimerId tid = base.runAfter(1000, []{ info("a second passed"); });
base.cancel(tid);
```

<h2 id="tcp-conn">TcpConn tcp connection</h2>
use shared_ptr to manage connection, no need to release manually

### reference count

```c
typedef std::shared_ptr<TcpConn> TcpConnPtr;
```

### state

```c
enum State { Invalid=1, Handshaking, Connected, Closed, Failed, };
```

### example

```c
TcpConnPtr con = TcpConn::createConnection(base, host, port);
con->onState([=](const TcpConnPtr& con) {
    info("onState called state: %d", con->getState());
});
con->onRead([](const TcpConnPtr& con){
    info("recv %lu bytes", con->getInput().size());
    con->send("ok");
    con->getInput().clear();
});

```

### reconnect setting

```c
//set reconnect. -1: no reconnect; 0 :reconnect now; other: wait millisecond; default -1
void setReconnectInterval(int milli);
```

### callback for idle

```c
void addIdleCB(int idle, const TcpCallBack& cb);

//close connection if idle for 30 seconds
con->addIdleCB(30, [](const TcpConnPtr& con)) { con->close(); });
```

### Message mode
you can onRead or onMsg to handle message

```c
//message callback, confict with onRead callback. You should set only one of these
//codec will be released when connection destroyed
void onMsg(CodecBase* codec, const MsgCallBack& cb);
//send message
void sendMsg(Slice msg);

con->onMsg(new LineCodec, [](const TcpConnPtr& con, Slice msg) {
    info("recv msg: %.*s", (int)msg.size(), msg.data());
    con->sendMsg("hello");
});

```

### store you own data

```c
template<class T> T& context();

con->context<std::string>() = "user defined data";
```

<h2 id="tcp-server">TcpServer</h2>
### example

```c
TcpServer echo(&base);
int r = echo.bind("", 2099);
exitif(r, "bind failed %d %s", errno, strerror(errno));
echo.onConnRead([](const TcpConnPtr& con) {
    con->send(con->getInput()); // echo data read
});
```

### customize your connection
when TcpServer accept a connection, it will call this to create an TcpConn

```
void onConnCreate(const std::function<TcpConnPtr()>& cb);

chat.onConnCreate([&]{
    TcpConnPtr con(new TcpConn);
    con->onState([&](const TcpConnPtr& con) {
        if (con->getState() == TcpConn::Connected) {
            con->context<int>() = 1;
        }
    }
    return con;
});
```

<h2 id="http-server">HttpServer</h2>

```c
//example
HttpServer sample(&base);
int r = sample.bind("", 8081);
exitif(r, "bind failed %d %s", errno, strerror(errno));
sample.onGet("/hello", [](const HttpConnPtr& con) {
   HttpResponse resp;
   resp.body = Slice("hello world");
   con.sendResponse(resp);
});

```
<h2 id="hsha">half sync half async server</h2>

```c
// empty string indicates unfinished handling of request. You may operate on con as you like.
void onMsg(CodecBase* codec, const RetMsgCallBack& cb);

hsha.onMsg(new LineCodec, [](const TcpConnPtr& con, const string& input){
    int ms = rand() % 1000;
    info("processing a msg");
    usleep(ms * 1000);
    return util::format("%s used %d ms", input.c_str(), ms);
});

```
updating.......
