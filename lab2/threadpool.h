#ifndef THREAD_POOL_H
#define THREAD_POOL_H

//#include "httpserver.h"
#include <vector>
#include <thread>
#include <queue>
#include <atomic>
#include <functional>
#include <condition_variable>
#include <future>
#include <stdexcept>
using namespace std;

const int THREADPOOL_MAX_NUM = 1250;

class threadpool
{
    using Task = function<void()>; //定义类型
    vector<thread> _pool;  //线程池
    queue<Task> _tasks;  //任务队列
    mutex _lock;
    condition_variable _task_cv;  //条件阻塞
    atomic<bool> _run{ true };    //线程池是否执行
    atomic<int> _idlThrNum{0}; //空闲线程数量

public:
    inline threadpool(unsigned short size = 4) { addThread(size); }
    inline ~threadpool()
    {
        _run = false;
        _task_cv.notify_all();
        for (thread& thread : _pool) {
            if (thread.joinable())
                thread.join();
        }
    }
    template<class F, class... Args>
    auto commit(F&& f, Args&&... args) ->future<decltype(f(args...))>
    {
        if (!_run)
            throw runtime_error("commit on ThreadPool is stopped.");

        using RetType = decltype(f(args...));
        auto task = make_shared<packaged_task<RetType()> >(
            std::bind(forward<F>(f), forward<Args>(args)...)
        );
        future<RetType> future = task->get_future();
        {
            lock_guard<mutex> lock{_lock};
            _tasks.emplace([task](){
                (*task)();
            });
        }
#ifdef THREADPOOL_AUTO_GROW
        if (_idlThrNum < 1 && _pool.size() < THREADPOOL_MAX_NUM)
            addThread(1);
#endif
        _task_cv.notify_one();
        return future;
    }
    //空闲线程数量
    int idCount() {
        return _idlThrNum;
    }
    int TaskCount() {
        return _tasks.size();
    }    
    //线程数
    int ithCount() {
        return _pool.size();
    }
#ifndef THREADPOOL_AUTO_GROW
private:
#endif
    void addThread(unsigned short size)
    {
        for(; _pool.size() < THREADPOOL_MAX_NUM && size > 0; --size)
        {
            _pool.emplace_back( [this]{
                while (_run)
                {
                    Task task;
                    {
                        unique_lock<mutex> lock{ _lock };
                        _task_cv.wait(lock, [this]{
                            return !_run || !_tasks.empty();
                        });
                        if (!_run && _tasks.empty())
                            return ;
                        task = move(_tasks.front());
                        _tasks.pop();
                    }
                    _idlThrNum--;
                    task();
                    _idlThrNum++;
                }
            });
            _idlThrNum++;
        }
    }
};


#endif