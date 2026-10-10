CC = cc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror -O2

.PHONY: all test clean
all: build/parent build/child

build:
	mkdir -p build

build/parent: src/parent.c src/common.h | build
	$(CC) $(CFLAGS) src/parent.c -o $@

build/child: src/child.c src/common.h | build
	$(CC) $(CFLAGS) src/child.c -o $@

test: all
	python3 tests/test_lab.py

clean:
	rm -f build/parent build/child
