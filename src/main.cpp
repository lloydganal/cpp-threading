#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

typedef bool (*JobFunction)();

struct JobQueue {
  private:
    std::queue<JobFunction> job_queue;
    std::mutex mutex;
    std::condition_variable cv_job_queued;

  public:
    void push_job(JobFunction job) {
        this->mutex.lock();
        {
            this->job_queue.push(job);
            this->cv_job_queued.notify_one();
        }
        this->mutex.unlock();
    }

    JobFunction pop_job() {
        JobFunction job = nullptr;
        this->mutex.lock();
        {
            job = this->job_queue.front();
            this->job_queue.pop();
        }
        this->mutex.unlock();
        return job;
    }

    void wait_for_job() {
        std::unique_lock<std::mutex> lk(this->mutex);
        this->cv_job_queued.wait(lk, [&]() { return !this->job_queue.empty(); });
    }

    bool is_empty() { return this->job_queue.empty(); }
};

int main() {
    JobQueue job_queue;

    job_queue.push_job([]() -> bool {
        std::cout << "Job 1 processing" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        return false;
    });

    std::thread worker([&]() {
        std::cout << "Worker - Start" << std::endl;
        bool exit = false;
        for (;;) {
            job_queue.wait_for_job();
            JobFunction job = job_queue.pop_job();
            if (job() == true) {
                exit = true;
            }

            if (exit && job_queue.is_empty()) {
                break;
            }
        }
        std::cout << "Worker - End" << std::endl;
    });

    job_queue.push_job([]() -> bool {
        std::cout << "Job 2 processing" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        return true;
    });

    job_queue.push_job([]() -> bool {
        std::cout << "Job 3 processing" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        return false;
    });

    worker.join();
    return 0;
}