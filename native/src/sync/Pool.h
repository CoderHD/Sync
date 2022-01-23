#pragma once
#include "Types.h"
#include <vector>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <functional>

namespace Sync {
    constexpr u32 POOL_SIZE = 5;

    using Task = std::function<void()>;

    struct Executor {
        std::condition_variable condition;
        std::mutex syncronize;
        std::atomic_bool running;
        std::thread thread;
        Task task;
        Executor();
        void run();
        void execute(Task task);
    };

    struct ExecutorPool {
        Executor executors[POOL_SIZE];
        ExecutorPool();
        bool execute(Task task);
        void join();
    };
};