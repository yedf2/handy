#pragma once
#include <thread>
#include <atomic>
#include <list>
#include <vector>
#include <functional>
#include <limits>
#include <condition_variable>
#include <mutex>
#include "util.h"

namespace handy {

template<typename T> struct SafeQueue: private std::mutex, private noncopyable {
    static const int wait_infinite = std::numeric_limits<int>::max();
    //0 不限制队列中的任务数
    SafeQueue(size_t capacity=0): capacity_(capacity), exit_(false) {}
    //队列满则返回false
    bool push(T&& v);
    //超时则返回T()
    T pop_wait(int waitMs=wait_infinite);
    //超时返回false
    bool pop_wait(T* v, int waitMs=wait_infinite);

    size_t size();
    void exit();
    bool exited() { return exit_; }
private: 
    std::list<T> items_;
    std::condition_variable ready_;
    size_t capacity_;
    std::atomic<bool> exit_;
    void wait_ready(std::unique_lock<std::mutex>& lk, int waitMs);
};

typedef std::function<void()> Task;
extern template class SafeQueue<Task>;

struct ThreadPool: private noncopyable {
    //创建线程池
    ThreadPool(int threads, int taskCapacity=0, bool start=true);
    ~ThreadPool();
    void start();
    ThreadPool& exit() { tasks_.exit(); return *this; }
    void join();

    //队列满返回false
    bool addTask(Task&& task);
    bool addTask(Task& task) { return addTask(Task(task)); }
    size_t taskSize() { return tasks_.size(); }
private:
    SafeQueue<Task> tasks_;
    std::vector<std::thread> threads_;
};

//以下为实现代码，不必关心
template<typename T> size_t SafeQueue<T>::size() {
    std::lock_guard<std::mutex> lk(*this);
    return items_.size();
}

template<typename T> void SafeQueue<T>::exit() {
    exit_ = true;
    std::lock_guard<std::mutex> lk(*this);
    ready_.notify_all();
}

template<typename T> bool SafeQueue<T>::push(T&& v) {
    std::lock_guard<std::mutex> lk(*this);
    if (exit_ || (capacity_ && items_.size() >= capacity_)) {
        return false;
    }
    items_.push_back(std::move(v));
    ready_.notify_one();
    return true;
}
template<typename T> void SafeQueue<T>::wait_ready(
    std::unique_lock<std::mutex>& lk, int waitMs)
{
    if (exit_ || !items_.empty()) {
        return;
    }
    if (waitMs == wait_infinite) {
        ready_.wait(lk, [this]{ return exit_ || !items_.empty(); });
    } else if (waitMs > 0){
        auto tp = std::chrono::steady_clock::now()
            + std::chrono::milliseconds(waitMs);
        while (ready_.wait_until(lk, tp) != std::cv_status::timeout 
            && items_.empty() && !exit_) {
        }
    }
}



template<typename T> bool SafeQueue<T>::pop_wait(T* v, int waitMs) {
    std::unique_lock<std::mutex> lk(*this);
    wait_ready(lk, waitMs);
    if (items_.empty()) {
        return false;
    }
    *v = std::move(items_.front());
    items_.pop_front();
    return true;
}

template<typename T> T SafeQueue<T>::pop_wait(int waitMs) {
    std::unique_lock<std::mutex> lk(*this);
    wait_ready(lk, waitMs);
    if (items_.empty()) {
        return T();
    }
    T r = std::move(items_.front());
    items_.pop_front();
    return r;
}

}
