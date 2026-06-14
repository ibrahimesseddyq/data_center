#include "datacenter/node/node.hpp"

#include <algorithm>
#include <utility>

namespace datacenter {

GPUNode::GPUNode(std::string node_id, int gpu_capacity)
    : node_id_(std::move(node_id)), gpu_capacity_(gpu_capacity) {}

bool GPUNode::can_assign(const Job& job) const {
    return job.required_gpus <= available_gpus();
}

bool GPUNode::assign(const Job& job) {
    if (!can_assign(job)) {
        return false;
    }
    current_load_ += job.required_gpus;
    RunningJob running{job, job.duration};
    running.job.status = JobStatus::Running;
    running_jobs_.push_back(std::move(running));
    return true;
}

std::vector<std::uint64_t> GPUNode::advance(std::chrono::seconds elapsed) {
    std::vector<std::uint64_t> completed;
    for (RunningJob& running : running_jobs_) {
        running.remaining -= elapsed;
    }

    // Partition finished jobs to the end, freeing their GPUs as we go.
    auto finished = std::remove_if(
        running_jobs_.begin(), running_jobs_.end(), [&](const RunningJob& running) {
            if (running.remaining <= std::chrono::seconds{0}) {
                current_load_ -= running.job.required_gpus;
                completed.push_back(running.job.id);
                return true;
            }
            return false;
        });
    running_jobs_.erase(finished, running_jobs_.end());
    return completed;
}

bool GPUNode::complete(std::uint64_t job_id) {
    auto it = std::find_if(
        running_jobs_.begin(), running_jobs_.end(),
        [job_id](const RunningJob& running) { return running.job.id == job_id; });
    if (it == running_jobs_.end()) {
        return false;
    }
    current_load_ -= it->job.required_gpus;
    running_jobs_.erase(it);
    return true;
}

}  // namespace datacenter
