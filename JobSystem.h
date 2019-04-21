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

    static const int WORKERS_COUNT = 12;

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

    int get_idle_worker() {
        for (int index = 0; index < WORKERS_COUNT; index++)
            if (workers[index].idle.load()) return index;
        return -1;
    }

    static void system_loop() {

        while (system_working.load()) {

            std::unique_lock<std::mutex> pending_jobs_guard(pending_jobs_mtx);

            while (!pending_jobs.empty()) {
                int worker_id = get_idle_worker();
                if (worker_id >= 0) {

                    auto job = pending_jobs.front();
                    pending_jobs.pop();
                    workers[worker_id].job = job;

                    std::unique_lock<std::mutex> worker_idle_lock(workers_updated_mtx);
                    workers[worker_id].idle.store(false);
                    worker_idle_lock.unlock();
                    // wake workers
                    workers_updated_cv.notify_all();
                } else {
                    break;
                }
            }

            pending_jobs_guard.unlock();

            std::unique_lock<std::mutex> lock(system_loop_updated_mtx);
            if (added_job) continue;
            system_loop_updated_cv.wait(lock, [] { return !added_job.load(); });
            added_job.store(false);
        }
    }

    static void worker_loop(const int worker_index) {

        auto &worker = workers[worker_index];

        while (system_working.load()) {

            if (!worker.idle.load()) {
                worker.job->execute();
                worker.job->done.store(true);
                worker.idle.store(true);

                // wake system loop
                system_loop_updated_cv.notify_all();
            }

            std::unique_lock<std::mutex> lck(workers_updated_mtx);
            workers_updated_cv.wait(lck, [worker_index] {
                // continue sleeping if no job assigned and system still working
                return !workers[worker_index].idle.load() || !system_working.load();
            });
            lck.unlock();
        }
    }

    // returns when pending jobs queue is empty and all workers are idle
    void wait_for_done() {
        while (system_working.load()) {

            // Wait untill job is done or added
            std::unique_lock<std::mutex> system_loop_lock(system_loop_updated_mtx);
            system_loop_updated_cv.wait(system_loop_lock);
            system_loop_lock.unlock();

            std::unique_lock<std::mutex> pending_jobs_lock(pending_jobs_mtx);
            bool no_jobs_to_dispatch = pending_jobs.empty();
            pending_jobs_lock.unlock();

            if (!no_jobs_to_dispatch) continue;

            bool all_idle = true;
            for (int index = 0; index < WORKERS_COUNT; index++) {
                if (!workers[index].idle.load()) {
                    all_idle = false;
                    break;
                }
            }

            if (all_idle) break;
        }
    }

    void dispatch(const std::shared_ptr<Job> &job) {
        std::lock_guard<std::mutex> lock(pending_jobs_mtx);
        pending_jobs.push(job);
        // wake system loop
        std::unique_lock<std::mutex> l(system_loop_updated_mtx);
        added_job.store(true);
        l.unlock();
        system_loop_updated_cv.notify_all();
    }

    void start() {

        if (system_working.load()) return;

        system_working.store(true);

        system_thread = std::thread(system_loop);
        for (int index = 0; index < WORKERS_COUNT; index++) workers[index].thread = std::thread(worker_loop, index);
    }

    void shutdown() {

        if (!system_working.load()) return;

        system_working.store(false);

        // wake workers
        workers_updated_cv.notify_all();
        // wake system loop
        system_loop_updated_cv.notify_all();

        system_thread.join();
        for (int index = 0; index < WORKERS_COUNT; index++) workers[index].thread.join();
    }
}

#endif //JOB_SYSTEM_JOBSYSTEM_H

