#include "datacenter/node/node.hpp"

#include <algorithm>
#include <string>
#include <utility>

#include "datacenter/common/sync_print.hpp"

namespace datacenter {

using namespace std::chrono_literals;

GPUNode::GPUNode(std::string node_id, int gpu_capacity)
    : node_id_(std::move(node_id)), gpu_capacity_(gpu_capacity) {}

GPUNode::~GPUNode() {
    stop();
}

void GPUNode::start() {
    if (running_.exchange(true)) {
        return;  // already running
    }
    thread_ = std::thread([this] { run(); });
}

void GPUNode::stop() {
    if (!running_.exchange(false)) {
        return;  // not running
    }
    if (thread_.joinable()) {
        thread_.join();
    }
}

bool GPUNode::try_assign(const Job& job) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (job.required_gpus > gpu_capacity_ - current_load_) {
        return false;
    }
    current_load_ += job.required_gpus;
    RunningJob running{job, job.duration};
    running.job.status = JobStatus::Running;
    running_jobs_.push_back(std::move(running));
    return true;
}

GPUNode::Snapshot GPUNode::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return Snapshot{node_id_, gpu_capacity_, current_load_, running_jobs_};
}

void GPUNode::run() {
    while (running_.load()) {
        std::this_thread::sleep_for(1s);

        std::vector<std::uint64_t> completed;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (RunningJob& running : running_jobs_) {
                running.remaining -= 1s;
            }
            auto finished = std::remove_if(
                running_jobs_.begin(), running_jobs_.end(),
                [&](const RunningJob& running) {
                    if (running.remaining <= 0s) {
                        current_load_ -= running.job.required_gpus;
                        completed.push_back(running.job.id);
                        return true;
                    }
                    return false;
                });
            running_jobs_.erase(finished, running_jobs_.end());
        }  // release lock before doing I/O

        for (std::uint64_t id : completed) {
            sync_print("[" + node_id_ + "] completed job " + std::to_string(id) +
                       " (GPUs freed)");
        }
    }
}

}  // namespace datacenter
