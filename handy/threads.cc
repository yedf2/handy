#include "threads.h"
#include <assert.h>
#include <utility>
using namespace std;

namespace handy {

template class SafeQueue<Task>;

ThreadPool::ThreadPool(int threads, int maxWaiting, bool start): 
tasks_(maxWaiting), threads_(threads)
{
    if (start) {
        this->start();
    }
}

ThreadPool::~ThreadPool() {
    assert(tasks_.exited());
    if (tasks_.size()) {
        fprintf(stderr, "%lu tasks not processed when thread pool exited\n",
            tasks_.size());
    }
}

void ThreadPool::start() {
    for (auto& th: threads_) {
        thread t(
            [this] {
                while (!tasks_.exited()) {
                    Task task;
                    if (tasks_.pop_wait(&task)) {
                        task();
                    }
                }
            }
        );
        th.swap(t);
    }
}

void ThreadPool::join() {
    for (auto& t: threads_) {
        t.join();
    }
}

bool ThreadPool::addTask(Task&& task) {
    return tasks_.push(move(task));
}

}
