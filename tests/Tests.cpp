//
// Created by dbeef on 4/21/19.
//

#include "gmock/gmock.h"
#include "JobSystem.hpp"

using ::testing::AtLeast;

class CounterJob : public Job {
public:

    int counter = 0;

    void execute() override {
        counter++;
    }

};

class ExpensiveJob : public Job {
public:

    void execute() override {
        for (volatile int a = 0; a < std::numeric_limits<uint16_t >::max(); a++) {
            volatile int i = i % 123456789;
        }
    }

};

TEST(JobSystemTest, testStartStop) {
    job_system::start();
    EXPECT_EQ(job_system::is_working(), true);
    job_system::shutdown();
    EXPECT_EQ(job_system::is_working(), false);
}

TEST(JobSystemTest, testLongAndInexpensiveTest_DispatchAndWait) {

    for(int x =0 ; x < 1000;x++) {
        job_system::start();

        for (int index = 0; index < 100; index++) {
            auto counter_job = std::make_shared<CounterJob>();
            job_system::dispatch(counter_job);
            job_system::wait_for_done();
            EXPECT_EQ(counter_job->counter, 1);
        }

        job_system::shutdown();
    }
}

TEST(JobSystemTest, testLongAndExpensiveTest_MultipleDispatchThenWait) {
    job_system::start();

    // create jobs
    std::vector<std::shared_ptr<Job>> expensive_jobs;

    for (int a = 0; a < 50000; a++) {
        auto job = std::make_shared<ExpensiveJob>();
        expensive_jobs.push_back(job);
        EXPECT_EQ(job->done.load(), false);
        job_system::dispatch(job);
    }

    job_system::wait_for_done();

    for(const auto& job : expensive_jobs) EXPECT_EQ(job->done.load(), true);

    job_system::shutdown();
}

int main(int argc, char **argv) {
    // The following line must be executed to initialize Google Mock
    // and Google Test before running tests.
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}