#pragma once

#include <chrono>
#include <cstdint>

namespace datacenter {

// Lifecycle state of a job within the scheduler.
enum class JobStatus {
    Pending,    // submitted, waiting to be scheduled
    Running,    // currently executing on a node
    Completed,  // finished successfully
    Failed,     // finished with an error
    Cancelled,  // cancelled before completion
};

// A unit of work submitted to the scheduler.
struct Job {
    std::uint64_t id = 0;                  // unique job identifier
    int required_gpus = 0;                 // number of GPUs the job needs
    std::chrono::seconds duration{0};      // estimated/requested run time
    int priority = 0;                      // higher value = scheduled sooner
    JobStatus status = JobStatus::Pending; // current lifecycle state
};

}  // namespace datacenter
