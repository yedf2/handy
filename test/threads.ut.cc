#include <handy/threads.h>
#include <unistd.h>
#include "test_harness.h"

using namespace std;
using namespace handy;

TEST(test::TestBase, ThreadPool) {
    ThreadPool pool(2, 5, false);
    int processed = 0;
    int* p = &processed;
    int added = 0;
    for (int i = 0; i < 10; i ++) {
        added += pool.addTask([=]{ printf("task %d processed\n", i);++*p; });
    }
    pool.start();
    usleep(100*1000);
    pool.exit();
    pool.join();
    ASSERT_EQ(added, processed);
    printf("pool tested\n");
    ThreadPool pool2(2);
    usleep(100*1000);
    processed = 0;
    added = 0;
    for (int i = 0; i < 10; i ++) {
        added += pool2.addTask([=]{ printf("task %d processed\n", i);++*p; });
    }
    usleep(100*1000);
    pool2.exit();
    pool2.join();
    ASSERT_EQ(added, processed);
}

TEST(test::TestBase, SafeQueue) {
    SafeQueue<int> q;
    atomic<bool> exit(false);
    q.push(-1);
    thread t([&]{ 
        while (!exit.load(memory_order_relaxed)) {
            int v = q.pop_wait(50);
            if (v) {
                printf("%d recved in consumer\n", v);
            }
            bool r = q.pop_wait(&v, 50);
            if (r) {
                printf("%d recved in consumer use pointer\n", v);
            }
        }
    });
    usleep(300*1000);
    printf("push other values\n");
    for (int i = 0; i < 5; i++) {
        q.push(i+1);
    }
    usleep(300*1000);
    exit.store(true, memory_order_relaxed);
    t.join();
    ASSERT_EQ(q.size(), 0);
}

