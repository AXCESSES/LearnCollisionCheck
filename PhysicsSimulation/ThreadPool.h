#pragma once
#include <thread>
#include <queue>
#include <functional>
#include <future>
#include <vector>
#include <condition_variable>

#include "SafeQueue.h"

class SimpleThreadPool
{
public:
    explicit SimpleThreadPool(size_t size = std::thread::hardware_concurrency());
    template<typename F, typename... Args>
    auto enqueue(F&& f, Args &&...args);
    ~SimpleThreadPool();

private:
    std::vector<std::thread> workers;
    std::queue <std::function<void()>> tasks;
    std::mutex queue_mtx;
    std::condition_variable condition;
    bool stop;
};

inline SimpleThreadPool::SimpleThreadPool(size_t size) :stop(false) {
    for (size_t i = 0; i < size; ++i) {
        workers.emplace_back(
            [this]() {
                // 一个循环，用于从任务队列中取出第一个任务
                while (true) {
                    std::function<void()> task; // 用于执行的任务
                    {
                        std::unique_lock<std::mutex> lk(queue_mtx); // 上个锁
                        // 解锁并等待直至线程池停止或任务队列不空
                        this->condition.wait(lk, [this]() {
                            return this->stop || !this->tasks.empty();
                                             });
                        // 如果线程池应当停止且任务为空，那么就停止并返回
                        if (this->stop && this->tasks.empty())return;
                        // 否则从队首取出一个任务
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task(); // 执行任务
                }
            }
        );
    }
}

template<typename F, typename ...Args>
inline auto SimpleThreadPool::enqueue(F&& f, Args && ...args) {
    using return_type = std::invoke_result_t<F, Args...>;
    auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lk(queue_mtx);
        if (stop) {
            throw std::runtime_error("enqueue on stopped Thread pool");
        }
        tasks.emplace([task = std::move(task)]() {(*task)(); });
    }
    condition.notify_one();
    return res;
}

inline SimpleThreadPool::~SimpleThreadPool() {
    {
        std::unique_lock<std::mutex> lk(queue_mtx);
        stop = true;
    }
    condition.notify_all();
    for (auto& worker : workers)worker.join();
}

class SafeSimpleThreadPool
{
private:
    using WorkItem = std::function<void()>;
    SafeQueue<WorkItem> q;
    std::vector<std::thread> workers;
    size_t threadCount;
public:
    explicit SafeSimpleThreadPool(size_t size = std::thread::hardware_concurrency()): threadCount(size) {
        for (size_t i = 0; i < size; ++i) {
            workers.emplace_back(
                [this]() {
                    while (true) {
                        WorkItem task;
                        if (!q.pop(task))return;
                        if (task)task();
                    }
                }
            );
        }
    }

    ~SafeSimpleThreadPool() {
        q.stop();
        for (auto& worker : workers)
            worker.join();
    }

    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&...args) {
        using return_type = std::invoke_result_t<F, Args...>;
        auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<return_type> res = task->get_future();
        q.push([task = std::move(task)]() {
            (*task)();
               });
        return res;
    }

    size_t getThreadCount()const {
        return threadCount;
    }

    template<typename F>
    void dispatch(size_t element_count, F&& f) {
        using return_type = std::invoke_result_t<F, size_t, size_t>;
        const size_t batch_size = element_count / threadCount;
        std::vector<std::future<return_type>> rets;
        rets.reserve(threadCount);
        for (size_t i = 0; i < threadCount; ++i) {
            const size_t start = batch_size * i;
            const size_t end = start + batch_size;
            rets.emplace_back(this->enqueue(f, start, end));
        }

        if (batch_size * threadCount < element_count) {
            const size_t start = batch_size * threadCount;
            f(start, element_count);
        }

        for (auto& ret : rets)ret.wait();
    }
};


class FastThreadPool
{
public:
    class TaskQueue
    {
    protected:
        using TaskType = std::function<void()>;
        std::queue<TaskType> m_tasks;
        std::mutex m_mutex;
        std::atomic<size_t> m_remaining_tasks = 0;
    public:
        template <typename F>
        void enqueue(F&& callback) {
            std::lock_guard<std::mutex> lk{ m_mutex };
            m_tasks.push(std::forward<F>(callback));
            m_remaining_tasks++;
        }

        void getTask(std::function<void()>& target_callback) {
            std::lock_guard<std::mutex> lk{ m_mutex };
            if (m_tasks.empty())return;
            target_callback = std::move(m_tasks.front());
            m_tasks.pop();
        }

        void waitForCompletion() const{
            while (m_remaining_tasks > 0)
                std::this_thread::yield();
        }

        void workDone() {
            m_remaining_tasks--;
        }
    };

    class Worker
    {
    protected:
        size_t m_id = 0;
        std::thread m_thread;
        std::function<void()> m_task = nullptr;
        bool running = true;
        TaskQueue* m_queue;
    public:
        //Worker() = default;
        Worker(TaskQueue& queue, size_t id): m_id(id), m_queue(&queue) {
            m_thread = std::thread(
                [this] {
                    run();
                }
            );
        }

        void run() {
            while (running) {
                m_queue->getTask(m_task);
                if (m_task == nullptr) {
                    std::this_thread::yield();
                }
                else {
                    m_task();
                    m_queue->workDone();
                    m_task = nullptr;
                }
            }
        }

        void stop() {
            running = false;
            m_thread.join();
        }

    };

protected:
    size_t m_thread_count = 0;
    TaskQueue m_queue;
    std::vector<Worker> m_workers;
public:
    explicit FastThreadPool(size_t thread_count) :m_thread_count(thread_count) {
        m_workers.reserve(thread_count);
        for (size_t i = thread_count; --i;) {
            m_workers.emplace_back(m_queue, m_workers.size());
        }
    }

    virtual ~FastThreadPool() {
        for (auto& worker : m_workers)worker.stop();
    }

    template<typename F>
    void enqueue(F&& f) {
        m_queue.enqueue(std::forward<F>(f));
    }

    void waitForCompletion() const {
        m_queue.waitForCompletion();
    }

    template<typename F>
    void dispatch(size_t element_count, F&& f) {
        const size_t batch_size = element_count / m_thread_count;
        for (size_t i = 0; i < m_thread_count; ++i) {
            const size_t start = batch_size * i;
            const size_t end = start + batch_size;
            this->enqueue([start, end, &f]() {f(start, end); });
        }
        if (batch_size * m_thread_count < element_count) {
            const size_t start = batch_size * m_thread_count;
            f(start, element_count);
        }

        waitForCompletion();
    }

    size_t getThreadCount() const {
        return m_thread_count;
    }
};