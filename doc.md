#handy
yedongfu

Handy是一个简洁高效的C++11网络库，支持linux与mac平台，使用异步IO模型

[使用示例](#sample)
[EventBase事件分发器](#event-base)
[tcp连接](#tcp-conn)
[tcp服务器](#tcp-server)
[http服务器](#http-server)
<h2 id="sample">使用示例--echo</h2>
```c
#include <handy/handy.h>

using namespace std;
using namespace handy;


int main(int argc, const char* argv[]) {
    EventBase base; //事件分发器
    //注册Ctrl+C的信号处理器--退出事件分发循环
    Signal::signal(SIGINT, [&]{ base.exit(); }); 
    TcpServer echo(&base); //创建服务器
    int r = echo.bind("", 99); //绑定端口
    exitif(r, "bind failed %d %s", errno, strerror(errno));
    echo.onConnRead([](const TcpConnPtr& con) {
        con->send(con->getInput()); // echo 读取的数据
    });
    base.loop(); //进入事件分发循环
}
```
##EventBase事件分发器
EventBase是事件分发器，内部使用epoll/kqueue来管理异步IO
```c
EventBase base;
```
###事件分发循环
```c
//不断调用epoll_wait，处理IO事件
base.loop();
```
###退出事件循环
```c
//退出事件循环，可在其他线程中调用
base.exit();
```
###是否已退出
```c
bool exited();
```
###管理客户端连接
```c
TcpConnPtr con = TcpConn::createConnection(&base, host, port);
```
###管理tcp服务器的所有socket
```c
TcpServer echo(&base);
```
###在IO线程中执行任务
一些任务必须在IO线程中完成，例如往连接中写入数据。非IO线程需要往连接中写入数据时，必须把任务交由IO线程进行处理
```c
void safeCall(const Task& task);

base.safeCall([](con){con->send("OK");});
```
###管理定时任务
EventBase通过设定epoll_wait/kevent的等待时间让自己及时返回，然后检查是否有到期的任务，因此时间精度依赖于epoll_wait/kevent的精度
```c
//interval: 0：一次性任务；>0：重复任务，每隔interval毫秒，任务被执行一次
TimerId runAfter(int64_t milli, const Task& task, int64_t interval=0);
//runAt则指定执行时刻
TimerId runAt(int64_t milli, const Task& task, int64_t interval=0)
//取消定时任务，若timer已经过期，则忽略
bool cancel(TimerId timerid);

TimerId tid = base.runAfter(1000, []{ info("a second passed"); });
base.cancel(tid);
```

<h2 id="tcp-conn">TcpConn tcp连接</h2>
连接采用引用计数的方式进行管理，因此用户无需手动释放连接
###引用计数
```c
typedef std::shared_ptr<TcpConn> TcpConnPtr;
```
###状态
```c
enum State { Invalid=1, Handshaking, Connected, Closed, Failed, };
```
###使用示例
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
###连接空闲回调
```c
void addIdleCB(int idle, const TcpCallBack& cb);

//连接空闲30s关闭连接
con->addIdleCB(30, [](const TcpConnPtr& con)) { con->close(); });
```
###消息模式
可以使用onRead处理消息，也可以选用onMsg方式处理消息
```c
//消息回调，此回调与onRead回调只有一个生效，后设置的生效
//codec所有权交给onMsg
void onMsg(CodecBase* codec, const MsgCallBack& cb);
//发送消息
void sendMsg(Slice msg);

con->onMsg(new LineCodec, [](const TcpConnPtr& con, Slice msg) {
    info("recv msg: %.*s", (int)msg.size(), msg.data());
    con->sendMsg("hello");
});
```
###存放自定义数据
```c
template<class T> T& context();

con->context<std::string>() = "user defined data";
```

<h2 id="tcp-server">TcpServer tcp服务器<h2>
###使用示例
```c
TcpServer echo(&base); //创建服务器
int r = echo.bind("", 99); //绑定端口
exitif(r, "bind failed %d %s", errno, strerror(errno));
echo.onConnRead([](const TcpConnPtr& con) {
    con->send(con->getInput()); // echo 读取的数据
});
```
###自定义创建的连接
当服务器accept一个连接时，调用此函数
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

<h2 id="http-server">HttpServer http服务器</h2>
```c
//使用示例
HttpServer sample(&base);
int r = sample.bind("", 8081);
exitif(r, "bind failed %d %s", errno, strerror(errno));
sample.onGet("/hello", [](const HttpConnPtr& con) {
   HttpResponse resp;
   resp.body = Slice("hello world");
   con.sendResponse(resp);
});
```

