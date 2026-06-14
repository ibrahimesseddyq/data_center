#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "datacenter/job/job.hpp"

namespace datacenter {

// A compute node exposing a fixed number of GPUs that runs jobs.
class GPUNode {
public:
    GPUNode(std::string node_id, int gpu_capacity);

    // --- accessors ---
    const std::string& node_id() const { return node_id_; }
    int gpu_capacity() const { return gpu_capacity_; }
    int current_load() const { return current_load_; }  // GPUs in use
    int available_gpus() const { return gpu_capacity_ - current_load_; }
    const std::vector<Job>& running_jobs() const { return running_jobs_; }

    // True if the node has enough free GPUs to run `job`.
    bool can_assign(const Job& job) const;

    // Places `job` on the node, marking it Running. Returns false (no-op) if it
    // does not fit.
    bool assign(const Job& job);

    // Removes a running job by id and frees its GPUs. Returns true if found.
    bool complete(std::uint64_t job_id);

private:
    std::string node_id_;
    int gpu_capacity_;
    int current_load_ = 0;
    std::vector<Job> running_jobs_;
};

}  // namespace datacenter
