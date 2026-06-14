# datacenter

A small, modular C++ GPU job scheduler. Jobs are submitted to a scheduler,
queued FIFO, and placed on the first GPU node with enough free capacity.

## Components

| Component | Location | Responsibility |
|-----------|----------|----------------|
| `Job` | `include/datacenter/job/job.hpp` | A unit of work: id, required GPUs, duration, priority, status. |
| `GPUNode` | `include/datacenter/node/node.hpp` | A node with a fixed GPU capacity that tracks load and running jobs. |
| `Scheduler` | `include/datacenter/scheduler/scheduler.hpp` | Queues jobs and assigns them to nodes (FIFO). |

### `Job`

A plain struct describing requested work:

```cpp
struct Job {
    std::uint64_t   id            = 0;
    int             required_gpus = 0;
    std::chrono::seconds duration{0};
    int             priority      = 0;            // higher = scheduled sooner
    JobStatus       status        = JobStatus::Pending;
};
```

`JobStatus` is `Pending | Running | Completed | Failed | Cancelled`.

### `GPUNode`

Owns its GPU capacity and the jobs currently running on it:

- `can_assign(job)` — does the job fit in the free GPUs?
- `assign(job)` — place the job, bump `current_load`, mark it `Running`.
- `complete(job_id)` — remove a job and free its GPUs.
- `available_gpus()` — `capacity − current_load`.

### `Scheduler`

- `add_node(node)` — register a node.
- `submit(job)` — enqueue a job.
- `schedule_next()` — place the oldest queued job on the first node that fits.
- `schedule_all()` — drain the queue until it empties or the head job fits no node.

The scheduler is **strict FIFO**: if the oldest job fits no node, it stays at the
front rather than letting smaller jobs jump ahead (no reordering, no starvation).

## Example

```cpp
#include "datacenter/scheduler/scheduler.hpp"

using namespace datacenter;

int main() {
    Scheduler scheduler;
    scheduler.add_node(GPUNode{"node-1", /*gpu_capacity=*/8});
    scheduler.add_node(GPUNode{"node-2", /*gpu_capacity=*/4});

    scheduler.submit(Job{.id = 1, .required_gpus = 4});
    scheduler.submit(Job{.id = 2, .required_gpus = 2});

    scheduler.schedule_all();  // both jobs placed on node-1
    return 0;
}
```

## Layout

```
include/datacenter/   public headers, one folder per module
  ├── job/            Job struct + JobStatus
  ├── node/           GPUNode
  └── scheduler/      Scheduler
src/                  implementations (one library per module)
apps/                 executables (scheduler_daemon, node_agent)
tests/                unit tests (mirrors src/)
examples/             usage examples
benchmarks/           performance tests
configs/              runtime configuration
cmake/                build helpers
docs/                 architecture notes
```

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

## Test

```sh
ctest --test-dir build --output-on-failure
```

## Requirements

- C++20 compiler (GCC 11+, Clang 14+, or MSVC 2022)
- CMake 3.20+
