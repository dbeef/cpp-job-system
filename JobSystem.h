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
#include <iostream>

class Job {
public:
    virtual void execute() = 0;
};

class Worker {
public:
    std::atomic_bool idle{true};
    std::thread thread;
    std::shared_ptr<Job> job;
};

namespace job_system {

    static const int WORKERS_COUNT = 2;

    std::atomic_bool system_working{false};

    std::mutex pending_jobs_mtx;
    std::queue<std::shared_ptr<Job>> pending_jobs;

    std::mutex main_loop_updated_mtx;
    std::condition_variable main_loop;

    std::mutex job_system_updated_mtx;
    std::condition_variable job_system_updated;

    std::array<Worker, WORKERS_COUNT> workers;

    std::thread system_thread;

    int get_idle_worker() {
        for (int index = 0; index < WORKERS_COUNT; index++) {
            if (workers[index].idle.load())
                return index;
        }
        return -1;
    }

    static void system_loop() {

        while (system_working.load()) {

            std::unique_lock<std::mutex> pending_jobs_guard(pending_jobs_mtx);

            if (!pending_jobs.empty()) {
                int worker_id = get_idle_worker();
                if (worker_id >= 0) {
                    std::cout << "Giving work to worker: " << worker_id << std::endl;
                    auto job = pending_jobs.front();
                    pending_jobs.pop();
                    workers[worker_id].job = job;

                    std::unique_lock<std::mutex> job_lock(job_system_updated_mtx);
                    workers[worker_id].idle.store(false);
                    std::cout << "System loop notifies" << std::endl;
                    job_lock.unlock();
                    job_system_updated.notify_all();
                } else {
                    std::cout << "Could not find idle worker" << std::endl;
                }
            }

            pending_jobs_guard.unlock();

            std::unique_lock<std::mutex> lock(main_loop_updated_mtx);
            main_loop.wait(lock);
            // do nothing, needed only for synchronization
            lock.unlock();
        }
    }

    static void worker_loop(const int worker_index) {

        auto &worker = workers[worker_index];
        std::cout << "Worker " << worker_index << " enters worker loop." << std::endl;

        while (system_working.load()) {

            if (!worker.idle.load()) {
                std::cout << "Worker " << worker_index << " executes job" << std::endl;
                worker.job->execute();
                worker.idle.store(true);
                std::cout << "Worker " << worker_index << " notifies" << std::endl;
                main_loop.notify_one();
            } else{
                std::cout << "Worker " << worker_index << " has no job" << std::endl;
            }

            std::cout << "Worker " << worker_index << " retrieving lock" << std::endl;
            std::unique_lock<std::mutex> lck(job_system_updated_mtx);
            std::cout << "Worker " << worker_index << " waits for update" << std::endl;
            job_system_updated.wait(lck, [worker_index] {
                std::cout << "IDLE: " << workers[worker_index].idle.load() << std::endl;
                bool result = (!workers[worker_index].idle.load() || !system_working.load());
                if(!result) std::cout << "Still sleeping " << workers[worker_index].idle.load() << " " << system_working.load() << std::endl;
                else std::cout << "Finishing sleeping " << workers[worker_index].idle.load() << " " << system_working.load() << std::endl;
                return result;}
                );
            lck.unlock();
        }

        std::cout << "Worker " << worker_index << " exits worker loop." << std::endl;
    }

    void wait_for_done() {

        while (system_working.load()) {

            std::unique_lock<std::mutex> pending_jobs_lock(pending_jobs_mtx);
            bool no_jobs_to_dispatch = pending_jobs.empty();
            pending_jobs_lock.unlock();

            bool all_idle = true;
            for (int index = 0; index < WORKERS_COUNT; index++) {
                if (!workers[index].idle.load()) {
                    all_idle = false;
                    break;
                }
            }

            if (all_idle && no_jobs_to_dispatch) {
                break;
            }
        }
    }

    void dispatch(const std::shared_ptr<Job> &job) {
        {
            std::cout << "Adding job" << std::endl;
            std::lock_guard<std::mutex> lock(pending_jobs_mtx);
            pending_jobs.push(job);
        }
        {
            main_loop.notify_one();
        }
    }

    void start() {

        std::cout << "Starting job system." << std::endl;

        if (system_working.load()) {
            std::cout << "System already working" << std::endl;
            return;
        }

        system_working.store(true);
        system_thread = std::thread(system_loop);

        for (int index = 0; index < WORKERS_COUNT; index++) {
            workers[index].thread = std::thread(worker_loop, index);
        }
    }

    void shutdown() {

        if (!system_working.load()) {
            std::cout << "System already shutdown" << std::endl;
            return;
        }

        std::cout << "Shutdown waiting for lock" << std::endl;
        std::unique_lock<std::mutex> lock(job_system_updated_mtx);
        system_working.store(false);
        lock.unlock();
        std::cout << "Shutdown notifies" << std::endl;
        job_system_updated.notify_all();

        std::cout << "Waiting for system thread to join" << std::endl;
        main_loop.notify_one();
        system_thread.join();
        for (int index = 0; index < WORKERS_COUNT; index++) {
            std::cout << "Shutdown notifies" << std::endl;
            job_system_updated.notify_all();
            std::cout << "Waiting for worker: " << index << " to join." << std::endl;
            workers[index].thread.join();
        }

    }
}

#endif //JOB_SYSTEM_JOBSYSTEM_H

