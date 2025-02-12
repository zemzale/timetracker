.PHONY: all
all: build

.PHONY: build
build: 
	mkdir -p build
	cd build && cmake ..
	cd build && cmake --build .

.PHONY: install
install: build
	cp ./build/timetracker ~/.local/bin/
