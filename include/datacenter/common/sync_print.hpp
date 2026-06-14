#pragma once

#include <iostream>
#include <mutex>
#include <string>

namespace datacenter {

// Prints a whole line atomically so output from multiple threads never
// interleaves mid-line.
inline void sync_print(const std::string& line) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    std::cout << line << '\n';
}

}  // namespace datacenter
