//
// Created by dbeef on 4/21/19.
//

#ifndef JOB_SYSTEM_JOBSYSTEM_H
#define JOB_SYSTEM_JOBSYSTEM_H

#include <memory>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>
#include <atomic>

class Job {
public:
    std::atomic_bool done{false};

    virtual void execute() = 0;
};

class Worker {
public:
    std::atomic_bool idle{true};
    std::thread thread;
    std::shared_ptr<Job> job;
};

class JobSystem {
public:

    // returns when pending jobs queue is empty and all workers are idle
    void wait_for_done();

    // adds job to queue
    void dispatch(const std::shared_ptr<Job> &job);

    // starts system loop, does not need to be called before dispatching
    void start();

    // synchronous call; joins all workers.
    void shutdown();

    bool is_working();

private:

    static const int WORKERS_COUNT = 12;

    std::atomic_bool job_done{false};
    std::atomic_bool added_job{false};
    std::atomic_bool system_working{false};
    std::thread system_thread;
    std::array<Worker, WORKERS_COUNT> workers;

    // for accessing pending_jobs queue
    std::mutex pending_jobs_mtx;
    std::queue<std::shared_ptr<Job>> pending_jobs;

    // for synchronizing system loop with adding and finishing jobs
    std::mutex system_loop_updated_mtx;
    std::condition_variable system_loop_updated_cv;

    // for synchronizing worker loop with job dispatched from system loop
    std::mutex workers_updated_mtx;
    std::condition_variable workers_updated_cv;

    int get_idle_worker();

    void system_loop();

    void worker_loop(int worker_index);
};

#endif //JOB_SYSTEM_JOBSYSTEM_H

