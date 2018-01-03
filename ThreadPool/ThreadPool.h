//ThreadPool.h

#ifndef ThreadPool_h
#define ThreadPool_h

#include <iostream>
#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <future>
#include <functional>
#include <utility>
#include <mutex>
#include <stdexcept>
#include <condition_variable>

class ThreadPool
{
public:
    ThreadPool(size_t num);
    ~ThreadPool();
    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type>;
private:
    std::mutex queue_mutex;
    std::queue< std::function<void()> > taskQueue;
    std::vector<std::thread> workQueue;
    std::condition_variable cv;
    bool stop;
};

inline ThreadPool::ThreadPool(size_t num):stop(false)
{
    for(size_t i = 0; i < num; ++i)
    {
        workQueue.emplace_back([this]{
            while(true)
            {
                std::function<void()> task;
                std::unique_lock<std::mutex> lock(this->queue_mutex);
                this->cv.wait(lock, [this]{
                    return (this->stop || !this->taskQueue.empty());});
                if (this->stop && this->taskQueue.empty()) return;
                task = std::move(this->taskQueue.front());
                this->taskQueue.pop();
                task();
             }
        });
     }
}

inline ThreadPool::~ThreadPool()
{
    std::unique_lock<std::mutex> lock(queue_mutex);
    this->stop = true;
    this->cv.notify_all();
    for (std::thread &worker : workQueue) //finish rest job
    {
        worker.join();
    }
}

template <typename F, typename... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    using result_type = typename std::result_of<F(Args...)>::type;
    auto task = std::make_shared<std::packaged_task<result_type()> >(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    std::future<result_type> res = task->get_future();
    std::unique_lock<std::mutex> lock(queue_mutex);
    if (stop) throw std::runtime_error("enqueue on stopped ThreadPool");
    taskQueue.emplace([task]{ (*task)(); });
 
    cv.notify_one();
    return res;
}
   
#endif
