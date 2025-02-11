.PHONY: all
all: build

.PHONY: build
build: 
	mkdir -p build
	cd build && cmake ..
	cd build && cmake --build .
