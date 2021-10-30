#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
class ThreadPool {
public:
    //make_shared()是用来构造shared_ptr的
    explicit ThreadPool(size_t threadCount = 8): pool_(std::make_shared<Pool>()) {
            assert(threadCount > 0);

            // 创建threadCount个子线程
            for(size_t i = 0; i < threadCount; i++) {
                //使用lambda表达式创建线程
                std::thread([pool = pool_] {
                    std::unique_lock<std::mutex> locker(pool->mtx); //创建一个互斥锁
                    while(true) {
                        if(!pool->tasks.empty()) {
                            auto task = std::move(pool->tasks.front()); // 从任务队列中取第一个任务
                            pool->tasks.pop();   // 移除掉队列中第一个元素
                            
                            locker.unlock();  //解锁
                            
                            task();  //调用任务函数
                            
                            locker.lock();   //加锁
                        } 
                        else if(pool->isClosed) break;
                        else pool->cond.wait(locker);   // 如果队列为空，等待唤醒
                    }
                }).detach();// 线程分离
            }
    }

    ThreadPool() = default;

    ThreadPool(ThreadPool&&) = default;
    
    ~ThreadPool() {
        if(static_cast<bool>(pool_)) {
            {
                std::lock_guard<std::mutex> locker(pool_->mtx);
                pool_->isClosed = true;
            }
            pool_->cond.notify_all();
        }
    }

    template<class F>
    void AddTask(F&& task) {
        {
            std::lock_guard<std::mutex> locker(pool_->mtx); //加锁
            pool_->tasks.emplace(std::forward<F>(task));  //任务队列中添加一个任务，完美转发
        }
        pool_->cond.notify_one();   // 唤醒一个等待的线程
    }

private:
    // 结构体
    struct Pool {
        std::mutex mtx;     // 互斥锁
        std::condition_variable cond;   // 条件变量
        bool isClosed;          // 是否关闭
        std::queue<std::function<void()>> tasks;    // 队列（保存的是任务）
    };
    std::shared_ptr<Pool> pool_;  //  池子指针
};


#endif //THREADPOOL_H