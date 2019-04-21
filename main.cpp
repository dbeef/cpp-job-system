#include <iostream>
#include "JobSystem.h"

class PrintJob : public Job {
public:
    void execute() override {
        std::cout << "This is print job" << std::endl;
    }
};

int main() {
    std::cout << "***************" << std::endl;

    job_system::start();

    for(int a = 0 ;a < 200;a++) {
        std::cout << "***************" << std::endl;
        auto print_job = std::make_shared<PrintJob>();
        job_system::dispatch(print_job);
        std::cout << "***************" << std::endl;
    }
    job_system::wait_for_done();

    job_system::shutdown();
    std::cout << "***************" << std::endl;

    return 0;
}