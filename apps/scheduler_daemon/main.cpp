// Demo driver: submits a batch of jobs over time, runs the scheduler once per
// second, and prints cluster state after each tick.
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

#include "datacenter/scheduler/scheduler.hpp"

using namespace datacenter;
using namespace std::chrono_literals;

namespace {

// A job paired with the tick at which it should be submitted.
struct PendingSubmission {
    int submit_at_tick;
    Job job;
};

int running_job_count(const Scheduler& scheduler) {
    int total = 0;
    for (const GPUNode& node : scheduler.nodes()) {
        total += static_cast<int>(node.running_jobs().size());
    }
    return total;
}

void print_state(int tick, const Scheduler& scheduler) {
    std::cout << "==== tick " << tick << " ====\n";
    for (const GPUNode& node : scheduler.nodes()) {
        std::cout << "  " << node.node_id() << ": " << node.current_load() << "/"
                  << node.gpu_capacity() << " GPUs | jobs: [";
        bool first = true;
        for (const GPUNode::RunningJob& running : node.running_jobs()) {
            std::cout << (first ? "" : ", ") << running.job.id << "("
                      << running.remaining.count() << "s)";
            first = false;
        }
        std::cout << "]\n";
    }
    std::cout << "  pending queue: " << scheduler.pending_jobs() << " job(s)\n\n";
}

}  // namespace

int main() {
    Scheduler scheduler;
    scheduler.add_node(GPUNode{"node-1", /*gpu_capacity=*/8});
    scheduler.add_node(GPUNode{"node-2", /*gpu_capacity=*/4});

    // Jobs arrive over the first few ticks.
    std::vector<PendingSubmission> inbox = {
        {1, Job{.id = 1, .required_gpus = 4, .duration = 10s}},
        {1, Job{.id = 2, .required_gpus = 2, .duration = 5s}},
        {2, Job{.id = 3, .required_gpus = 4, .duration = 8s}},
        {3, Job{.id = 4, .required_gpus = 2, .duration = 3s}},
        {5, Job{.id = 5, .required_gpus = 6, .duration = 6s}},
    };

    constexpr int kMaxTicks = 30;
    for (int tick = 1; tick <= kMaxTicks; ++tick) {
        // 1. Advance time: complete finished jobs and free their GPUs.
        for (std::uint64_t done : scheduler.advance(1s)) {
            std::cout << "[tick " << tick << "] completed job " << done
                      << " (GPUs freed)\n";
        }

        // 2. Submit any jobs that arrive on this tick.
        for (const auto& entry : inbox) {
            if (entry.submit_at_tick == tick) {
                scheduler.submit(entry.job);
                std::cout << "[tick " << tick << "] submitted job " << entry.job.id
                          << " (gpus=" << entry.job.required_gpus << ")\n";
            }
        }

        // 3. Process scheduling.
        std::size_t placed = scheduler.schedule_all();
        if (placed > 0) {
            std::cout << "[tick " << tick << "] placed " << placed << " job(s)\n";
        }

        // 4. Print state.
        print_state(tick, scheduler);

        // Stop once nothing is left to submit, queued, or running.
        bool more_to_submit = false;
        for (const auto& entry : inbox) {
            if (entry.submit_at_tick > tick) {
                more_to_submit = true;
                break;
            }
        }
        if (!more_to_submit && scheduler.pending_jobs() == 0 &&
            running_job_count(scheduler) == 0) {
            std::cout << "all jobs completed; stopping.\n";
            break;
        }

        std::this_thread::sleep_for(1s);
    }

    return 0;
}
