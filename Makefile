.PHONY: all build test sim clean worker master cli proto android-build help

NCORES := $(shell nproc)
BUILD_DIR := build/linux
CONFIG_FLAGS ?=

help:
	@echo "Void_OS-Merger — make targets:"
	@echo "  make            # build everything"
	@echo "  make build      # same as above"
	@echo "  make test       # run the worker self-test"
	@echo "  make sim        # launch 1 master + 3 workers on localhost"
	@echo "  make worker     # build only cluster-worker"
	@echo "  make master     # build only cluster-master"
	@echo "  make cli        # build only vom-cli"
	@echo "  make proto      # regenerate Cap'n Proto bindings"
	@echo "  make clean      # wipe build/"

all build: proto
	@mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake ../.. $(CONFIG_FLAGS) && make -j$(NCORES)

proto:
	@bash scripts/gen_proto.sh

worker: proto
	@mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake ../.. $(CONFIG_FLAGS) && make -j$(NCORES) cluster-worker

master: proto
	@mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake ../.. $(CONFIG_FLAGS) && make -j$(NCORES) cluster-master

cli: proto
	@mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake ../.. $(CONFIG_FLAGS) && make -j$(NCORES) vom-cli

test: all
	$(BUILD_DIR)/cluster-worker --test

sim: all
	@bash scripts/sim_cluster.sh

android-build:
	@echo "TODO: wire up CMake/NDK toolchain for Android targets"

clean:
	rm -rf build/
