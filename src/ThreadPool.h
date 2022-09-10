#pragma once
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <atomic>
#include <future>
#include <queue>
#include <vector>
#include <condition_variable>
#include <thread>
#include <functional>
#include <stdexcept>

namespace std {
#define THREADPOOL_MAX_NUM 16

class threadpool {
  using Task = function<void()>; //定义类型
  vector<thread> _pool;          //线程池
  queue<Task> _tasks;            //任务队列
  mutex _lock;                   //同步
  condition_variable _task_cv;   //条件阻塞
  atomic<bool> _run{true};       //线程池是否执行
  atomic<int> _idlThrNum{0};     //空闲线程数量

public:
  inline threadpool(unsigned short size = 4) { addThread(size); }
  inline ~threadpool() {
    _run = false;
    _task_cv.notify_all(); // 唤醒所有线程执行
    for (thread &thread : _pool) {
      // thread.detach(); // 让线程“自生自灭”
      if (thread.joinable())
        thread.join(); // 等待任务结束， 前提：线程一定会执行完
    }
  }

public:
  template <class F, class... Args>
  auto commit(F &&f, Args &&... args) -> future<decltype(f(args...))> {
    if (!_run) // stoped ??
      throw runtime_error("commit on ThreadPool is stopped.");

    using RetType = decltype(f(args...)); 
    auto task = make_shared<packaged_task<RetType()>>(bind(
        forward<F>(f), forward<Args>(args)...)); // 把函数入口及参数,打包(绑定)
    future<RetType> future = task->get_future();
    { // 添加任务到队列
      lock_guard<mutex> lock{ _lock }; 
      _tasks.emplace([task]() { (*task)(); });
    }
#ifdef THREADPOOL_AUTO_GROW
    if (_idlThrNum < 1 && _pool.size() < THREADPOOL_MAX_NUM)
      addThread(1);
#endif                     // !THREADPOOL_AUTO_GROW
    _task_cv.notify_one(); // 唤醒一个线程执行

    return future;
  }

  int idlCount() { return _idlThrNum; }
  int thrCount() { return _pool.size(); }
#ifndef THREADPOOL_AUTO_GROW
private:
#endif // !THREADPOOL_AUTO_GROW
       //添加指定数量的线程
  void addThread(unsigned short size) {
    for (; _pool.size() < THREADPOOL_MAX_NUM && size > 0; --size) { //增加线程数量,但不超过 预定义数量 THREADPOOL_MAX_NUM
      _pool.emplace_back([this] { //工作线程函数
        while (_run) {
          Task task; // 获取一个待执行的 task
          {
            unique_lock<mutex> lock{_lock};
            _task_cv.wait(lock, [this] {
              return !_run || !_tasks.empty();
            }); // wait 直到有 task
            if (!_run && _tasks.empty())
              return;
            task = move(_tasks.front()); // 按先进先出从队列取一个 task
            _tasks.pop();
          }
          _idlThrNum--;
          task(); //执行任务
          _idlThrNum++;
        }
      });
      _idlThrNum++;
    }
  }
};

} // namespace std

#endif // https://github.com/lzpong/