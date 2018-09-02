#include <handy/conn.h>
#include <handy/logging.h>
#include <thread>
#include "test_harness.h"

using namespace std;
using namespace handy;

TEST(test::TestBase, Ip4Addr) {
    ASSERT_EQ("127.0.0.1:80", Ip4Addr("localhost", 80).toString());
    ASSERT_EQ(true, Ip4Addr("www.baidu.com", 80).isIpValid());
    ASSERT_EQ(Ip4Addr("127...", 80).isIpValid(), false);
    ASSERT_EQ(true, Ip4Addr("127.0.0.1", 80).isIpValid());
}

TEST(test::TestBase, EventBase) {
    EventBase base;
    base.safeCall([] { info("task by base.addTask"); });
    thread th([&] {
        usleep(50000);
        info("base exit");
        base.exit();
    });
    base.loop();
    th.join();
}

TEST(test::TestBase, Timer) {
    EventBase base;
    long now = util::timeMilli();
    info("adding timers ");
    TimerId tid1 = base.runAt(now + 100, [] { info("timer at 100"); });
    TimerId tid2 = base.runAfter(50, [] { info("timer after 50"); });
    TimerId tid3 = base.runAfter(20, [] { info("timer interval 10"); }, 10);
    base.runAfter(120, [&] {
        info("after 120 then cancel above");
        base.cancel(tid1);
        base.cancel(tid2);
        base.cancel(tid3);
        base.exit();
    });
    base.loop();
}

TEST(test::TestBase, TcpServer1) {
    EventBase base;
    ThreadPool th(2);
    TcpServer delayEcho(&base);
    int r = delayEcho.bind("", 2099);
    ASSERT_EQ(r, 0);
    delayEcho.onConnRead([&th, &base](const TcpConnPtr &con) {
        th.addTask([&base, con] {
            usleep(200 * 1000);
            info("in pool");
            base.safeCall([con, &base] {
                con->send(con->getInput());
                base.exit();
            });
        });
        con->close();
    });
    TcpConnPtr con = TcpConn::createConnection(&base, "localhost", 2099);
    con->onState([](const TcpConnPtr &con) {
        if (con->getState() == TcpConn::Connected)
            con->send("hello");
    });
    base.loop();
    th.exit();
    th.join();
}

TEST(test::TestBase, kevent) {
    EventBase base;
    TcpServer echo(&base);
    int r = echo.bind("", 2099);
    ASSERT_EQ(r, 0);
    echo.onConnRead([](const TcpConnPtr &con) { con->send(con->getInput()); });
    TcpConnPtr con = TcpConn::createConnection(&base, "localhost", 2099);
    con->onState([](const TcpConnPtr &con) {
        if (con->getState() == TcpConn::Connected)
            con->send("hello");
    });
    base.runAfter(5, [con, &base] {
        con->closeNow();
        base.exit();
    });
    base.loop();
}