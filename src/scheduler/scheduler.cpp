#include "datacenter/scheduler/scheduler.hpp"

#include <utility>

namespace datacenter {

Scheduler::~Scheduler() {
    stop_all();
}

void Scheduler::add_node(std::string node_id, int gpu_capacity) {
    auto node = std::make_unique<GPUNode>(std::move(node_id), gpu_capacity);
    node->start();
    nodes_.push_back(std::move(node));
}

void Scheduler::submit(const Job& job) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    queue_.push(job);
}

bool Scheduler::schedule_next() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    if (queue_.empty()) {
        return false;
    }

    const Job& job = queue_.front();
    for (auto& node : nodes_) {
        if (node->try_assign(job)) {  // thread-safe; no-op if it does not fit
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

std::size_t Scheduler::pending_jobs() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return queue_.size();
}

std::vector<GPUNode::Snapshot> Scheduler::snapshot() const {
    std::vector<GPUNode::Snapshot> out;
    out.reserve(nodes_.size());
    for (const auto& node : nodes_) {
        out.push_back(node->snapshot());
    }
    return out;
}

void Scheduler::stop_all() {
    for (auto& node : nodes_) {
        node->stop();
    }
}

}  // namespace datacenter
