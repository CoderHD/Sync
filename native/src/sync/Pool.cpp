#if defined(LIVE)
#include "Pool.h"
#include "Log.h"
#include <thread>

Sync::Executor::Executor() : running(false), thread(std::thread(std::bind(&Executor::run, this))), task(null) {}

void Sync::Executor::run() {
	while (true) {
		// Warte auf Notify
		{
			std::unique_lock<std::mutex> lock(syncronize);
			condition.wait(lock);
		}
		// Funktion ausf�hren
		running.store(true);
		if (!task) {
			LOG_ERROR("Task ist null");
			return;
		}
		task();
		running.store(false);
	}
}

void Sync::Executor::execute(Task task) {
	this->task = task;
	condition.notify_all();
	auto lambda = []() -> bool { return true; };
}

Sync::ExecutorPool::ExecutorPool() {}

bool Sync::ExecutorPool::execute(Task task) {
	for (int i = 0; i < POOL_SIZE; i++) {
		Executor& executor = executors[i];
		if (!executor.running.load()) {
			executor.execute(task);
			return true;
		}
	}
	return false;
}

void Sync::ExecutorPool::join() {
	executors[0].thread.join();
}
#endif