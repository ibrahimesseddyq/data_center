#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "datacenter/job/job.hpp"

namespace datacenter {

// A compute node with a fixed number of GPUs. Each node runs its own worker
// thread that advances its jobs and frees GPUs as they complete.
//
// Thread-safety: try_assign() and snapshot() may be called from any thread; the
// worker thread mutates state under the same lock. The object owns a thread and
// a mutex, so it is neither copyable nor movable.
class GPUNode {
public:
    // A job currently executing on the node, with the time left until it ends.
    struct RunningJob {
        Job job;
        std::chrono::seconds remaining;
    };

    // A consistent point-in-time view of the node, safe to read off-thread.
    struct Snapshot {
        std::string node_id;
        int gpu_capacity;
        int current_load;
        std::vector<RunningJob> running_jobs;
    };

    GPUNode(std::string node_id, int gpu_capacity);
    ~GPUNode();

    GPUNode(const GPUNode&) = delete;
    GPUNode& operator=(const GPUNode&) = delete;

    // Launches / stops the worker thread. stop() joins and is idempotent.
    void start();
    void stop();

    // Immutable after construction — no lock required.
    const std::string& node_id() const { return node_id_; }
    int gpu_capacity() const { return gpu_capacity_; }

    // Thread-safe. Places `job` if it fits; returns false otherwise.
    bool try_assign(const Job& job);

    // Thread-safe consistent view for reporting.
    Snapshot snapshot() const;

private:
    void run();  // worker loop: advances jobs once per second

    const std::string node_id_;
    const int gpu_capacity_;

    mutable std::mutex mutex_;            // guards the two fields below
    int current_load_ = 0;               // GPUs in use
    std::vector<RunningJob> running_jobs_;

    std::thread thread_;
    std::atomic<bool> running_{false};
};

}  // namespace datacenter
