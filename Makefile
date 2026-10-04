CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pthread
.PHONY: all driver app test clean
all: app
app: build/monitor
build/monitor: src/*.hpp src/main.cpp include/vs_ioctl.h
	@mkdir -p build
	$(CXX) $(CXXFLAGS) src/main.cpp -o $@
build/test_core: src/*.hpp tests/test_core.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) tests/test_core.cpp -o $@
test: build/test_core
	./build/test_core
driver:
	$(MAKE) -C driver
clean:
	rm -rf build
	-$(MAKE) -C driver clean
