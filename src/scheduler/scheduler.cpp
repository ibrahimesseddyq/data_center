#include "datacenter/scheduler/scheduler.hpp"

#include <utility>

namespace datacenter {

void Scheduler::add_node(GPUNode node) {
    nodes_.push_back(std::move(node));
}

void Scheduler::submit(const Job& job) {
    queue_.push(job);
}

bool Scheduler::schedule_next() {
    if (queue_.empty()) {
        return false;
    }

    const Job& job = queue_.front();
    for (GPUNode& node : nodes_) {
        if (node.assign(job)) {  // assign() is a no-op if the job does not fit
            queue_.pop();
            return true;
        }
    }

    // Head-of-queue job fits no node right now; leave it in place (FIFO).
    return false;
}

std::size_t Scheduler::schedule_all() {
    std::size_t placed = 0;
    while (schedule_next()) {
        ++placed;
    }
    return placed;
}

}  // namespace datacenter
