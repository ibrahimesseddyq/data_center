#pragma once

#include <cstddef>
#include <queue>
#include <vector>

#include "datacenter/job/job.hpp"
#include "datacenter/node/node.hpp"

namespace datacenter {

// FIFO scheduler: queues submitted jobs and places each (oldest first) on the
// first node with enough free GPUs.
class Scheduler {
public:
    // Registers a node the scheduler can place jobs on.
    void add_node(GPUNode node);

    // Enqueues a job for scheduling.
    void submit(const Job& job);

    // Tries to place the oldest queued job on the first available node.
    // Returns true if a job was placed; false if the queue is empty or the
    // head-of-queue job does not fit any node (FIFO order is preserved, so it
    // is not skipped).
    bool schedule_next();

    // Repeatedly places jobs until the queue empties or the head job no longer
    // fits any node. Returns the number of jobs placed.
    std::size_t schedule_all();

    std::size_t pending_jobs() const { return queue_.size(); }
    const std::vector<GPUNode>& nodes() const { return nodes_; }

private:
    std::vector<GPUNode> nodes_;
    std::queue<Job> queue_;
};

}  // namespace datacenter
