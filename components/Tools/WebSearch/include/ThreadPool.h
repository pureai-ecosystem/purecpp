#pragma once
#include <condition_variable>
#include <future>
#include <queue>
#include <thread>
#include <vector>

namespace purecpp::websearch {

/**
 * Thread-pool simples e _header-only_ inspirado em `std::async`.
 * Permite enfileirar qualquer callable que retorne R.
 */
class ThreadPool {
public:
    explicit ThreadPool(std::size_t n_workers =
        std::thread::hardware_concurrency())
        : stop_(false)
    {
        for (std::size_t i = 0; i < n_workers; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    Task task;
                    {
                        std::unique_lock<std::mutex> lk(mtx_);
                        cv_.wait(lk, [this] { return stop_ || !queue_.empty(); });
                        if (stop_ && queue_.empty())
                            return;
                        task = std::move(queue_.front());
                        queue_.pop();
                    }
                    task();  // executa fora do lock
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& th : workers_) th.join();
    }

    template <class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result_t<F, Args...>>
    {
        using Ret = typename std::invoke_result_t<F, Args...>;

        auto task_ptr = std::make_shared<std::packaged_task<Ret()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        {
            std::lock_guard<std::mutex> lk(mtx_);
            if (stop_) throw std::runtime_error("ThreadPool stopped");
            queue_.emplace([task_ptr] { (*task_ptr)(); });
        }
        cv_.notify_one();
        return task_ptr->get_future();
    }

private:
    using Task = std::function<void()>;

    std::vector<std::thread> workers_;
    std::queue<Task>         queue_;
    std::mutex               mtx_;
    std::condition_variable  cv_;
    bool                     stop_;
};

} // namespace purecpp::websearch
