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

namespace job_system {

    // returns when pending jobs queue is empty and all workers are idle
    void wait_for_done();

    // adds job to queue
    void dispatch(const std::shared_ptr<Job> &job);

    // starts system loop, does not need to be called before dispatching
    void start();

    // synchronous call; joins all workers.
    void shutdown();

    bool is_working();
}

#endif //JOB_SYSTEM_JOBSYSTEM_H

