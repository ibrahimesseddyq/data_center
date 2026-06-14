// Demo driver: submits a batch of jobs over time and runs the scheduler once
// per second. Each GPU node runs on its own thread and completes its jobs
// asynchronously, freeing GPUs as it goes.
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#include "datacenter/common/sync_print.hpp"
#include "datacenter/scheduler/scheduler.hpp"

using namespace datacenter;
using namespace std::chrono_literals;

namespace {

// A job paired with the tick at which it should be submitted.
struct PendingSubmission {
    int submit_at_tick;
    Job job;
};

int running_job_count(const std::vector<GPUNode::Snapshot>& snaps) {
    int total = 0;
    for (const auto& snap : snaps) {
        total += static_cast<int>(snap.running_jobs.size());
    }
    return total;
}

void print_state(int tick, const std::vector<GPUNode::Snapshot>& snaps,
                 std::size_t pending) {
    std::string out = "==== tick " + std::to_string(tick) + " ====\n";
    for (const auto& snap : snaps) {
        out += "  " + snap.node_id + ": " + std::to_string(snap.current_load) + "/" +
               std::to_string(snap.gpu_capacity) + " GPUs | jobs: [";
        bool first = true;
        for (const auto& running : snap.running_jobs) {
            out += (first ? "" : ", ") + std::to_string(running.job.id) + "(" +
                   std::to_string(running.remaining.count()) + "s)";
            first = false;
        }
        out += "]\n";
    }
    out += "  pending queue: " + std::to_string(pending) + " job(s)";
    sync_print(out);
}

}  // namespace

int main() {
    Scheduler scheduler;
    scheduler.add_node("node-1", /*gpu_capacity=*/8);
    scheduler.add_node("node-2", /*gpu_capacity=*/4);

    // Jobs arrive over the first few ticks.
    std::vector<PendingSubmission> inbox = {
        {1, Job{.id = 1, .required_gpus = 4, .duration = 10s}},
        {1, Job{.id = 2, .required_gpus = 2, .duration = 5s}},
        {2, Job{.id = 3, .required_gpus = 4, .duration = 8s}},
        {3, Job{.id = 4, .required_gpus = 2, .duration = 3s}},
        {5, Job{.id = 5, .required_gpus = 6, .duration = 6s}},
    };

    constexpr int kMaxTicks = 40;
    for (int tick = 1; tick <= kMaxTicks; ++tick) {
        // 1. Submit any jobs that arrive on this tick.
        for (const auto& entry : inbox) {
            if (entry.submit_at_tick == tick) {
                scheduler.submit(entry.job);
                sync_print("[tick " + std::to_string(tick) + "] submitted job " +
                           std::to_string(entry.job.id) +
                           " (gpus=" + std::to_string(entry.job.required_gpus) + ")");
            }
        }

        // 2. Process scheduling (nodes free their own GPUs on their threads).
        std::size_t placed = scheduler.schedule_all();
        if (placed > 0) {
            sync_print("[tick " + std::to_string(tick) + "] placed " +
                       std::to_string(placed) + " job(s)");
        }

        // 3. Print a consistent snapshot.
        auto snaps = scheduler.snapshot();
        std::size_t pending = scheduler.pending_jobs();
        print_state(tick, snaps, pending);

        // Stop once nothing is left to submit, queued, or running.
        bool more_to_submit = false;
        for (const auto& entry : inbox) {
            if (entry.submit_at_tick > tick) {
                more_to_submit = true;
                break;
            }
        }
        if (!more_to_submit && pending == 0 && running_job_count(snaps) == 0) {
            sync_print("all jobs completed; stopping.");
            break;
        }

        std::this_thread::sleep_for(1s);
    }

    scheduler.stop_all();
    return 0;
}
