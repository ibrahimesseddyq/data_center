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
    running_jobs_.push_back(job);
    running_jobs_.back().status = JobStatus::Running;
    return true;
}

bool GPUNode::complete(std::uint64_t job_id) {
    auto it = std::find_if(running_jobs_.begin(), running_jobs_.end(),
                           [job_id](const Job& j) { return j.id == job_id; });
    if (it == running_jobs_.end()) {
        return false;
    }
    current_load_ -= it->required_gpus;
    running_jobs_.erase(it);
    return true;
}

}  // namespace datacenter
