# Simple Makefile for quick local builds.
# For the full/configurable build, use CMake instead (see README).

CXX      ?= g++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -Wpedantic -O2 -pthread
LDFLAGS  ?= -pthread
INCLUDES := -Iinclude

BUILD_DIR := build
BIN       := $(BUILD_DIR)/scheduler_daemon

# Library sources (job is header-only) + the daemon entry point.
SRCS := src/node/node.cpp \
        src/scheduler/scheduler.cpp \
        apps/scheduler_daemon/main.cpp

OBJS := $(SRCS:%.cpp=$(BUILD_DIR)/%.o)

.PHONY: all run clean

all: $(BIN)

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ $(LDFLAGS)

# Compile each .cpp into build/, mirroring the source tree.
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

run: $(BIN)
	./$(BIN)

clean:
	rm -rf $(BUILD_DIR)
