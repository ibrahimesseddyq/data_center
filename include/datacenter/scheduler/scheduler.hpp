#pragma once

#include <cstddef>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include "datacenter/job/job.hpp"
#include "datacenter/node/node.hpp"

namespace datacenter {

// FIFO scheduler over a pool of self-running GPU nodes. Jobs are queued and the
// oldest is placed on the first node with enough free GPUs. Nodes complete jobs
// on their own threads, freeing capacity asynchronously.
class Scheduler {
public:
    Scheduler() = default;
    ~Scheduler();

    // Creates a node, starts its worker thread, and registers it.
    void add_node(std::string node_id, int gpu_capacity);

    // Thread-safe: enqueues a job for scheduling.
    void submit(const Job& job);

    // Tries to place the oldest queued job on the first available node.
    // Returns true if a job was placed; false if the queue is empty or the
    // head-of-queue job fits no node (FIFO order is preserved).
    bool schedule_next();

    // Places jobs until the queue empties or the head job no longer fits any
    // node. Returns the number of jobs placed.
    std::size_t schedule_all();

    std::size_t pending_jobs() const;

    // Consistent point-in-time view of every node, safe to read off-thread.
    std::vector<GPUNode::Snapshot> snapshot() const;

    // Stops all node worker threads (also done by the destructor).
    void stop_all();

private:
    std::vector<std::unique_ptr<GPUNode>> nodes_;
    std::queue<Job> queue_;
    mutable std::mutex queue_mutex_;
};

}  // namespace datacenter
