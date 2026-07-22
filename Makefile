.PHONY: all agent-linux clean test

NCORES := $(shell nproc)

all: agent-linux

agent-linux:
	mkdir -p build/linux
	cd build/linux && cmake ../../agent -DPLATFORM=LINUX && make -j$(NCORES)

test: agent-linux
	build/linux/cluster-agent --test

clean:
	rm -rf build/
