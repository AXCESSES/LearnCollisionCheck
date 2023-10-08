#pragma once

#include <mutex>
#include <queue>
#include <condition_variable>

template<typename T>
class SafeQueue
{
private:
    std::queue<T> q;
    std::condition_variable condition;
    mutable std::mutex mtx;
    bool _stop = false;
public:
    SafeQueue() = default;
    ~SafeQueue() {
        stop();
    }
    void push(const T& val) {
        {
            std::unique_lock<std::mutex>lk(mtx);
            q.push(val);
        }
        condition.notify_one();
    }

    void push(T&& val) {
        {
            std::unique_lock<std::mutex> lk(mtx);
            q.push(std::move(val));
        }
        condition.notify_one();
    }

    bool pop(T& val) {
        std::unique_lock<std::mutex> lock(mtx);
        condition.wait(lock, [&]() {
            return !q.empty() || _stop;
                       });
        if (q.empty())return false;
        val = std::move(q.front());
        q.pop();
        return true;
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lk(mtx);
        return q.size();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lk(mtx);
        return q.empty();
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lk(mtx);
            _stop = true;
        }
        condition.notify_all();
    }
};