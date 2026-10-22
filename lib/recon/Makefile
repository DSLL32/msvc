CC ?= gcc
CPPFLAGS += -DRECON_NATIVE_TEST -Iinclude -Inative
CFLAGS += -m32 -std=gnu89 -O0 -g -Wall -Wextra
SOURCES = src/crt.c src/win32.c
HEADERS = $(wildcard include/*.h native/*.h)
.PHONY: all check trace-check clean
all: build/check
build:
	mkdir -p $@
build/check: tests/check.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/check.c $(SOURCES) -o $@
check: build/check
	./build/check
build/trace-target.o: tests/trace-target.c | build
	$(CC) -m32 -std=gnu89 -O0 -g -finstrument-functions -c $< -o $@
build/trace-pie: build/trace-target.o src/trace.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIE -pie $^ -o $@
build/trace-exec: build/trace-target.o src/trace.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -no-pie $^ -o $@
trace-check: build/trace-pie build/trace-exec
	python3 tests/check-trace.py build/trace-pie build/trace-exec
clean:
	rm -rf build
