CC ?= gcc
PYTHON ?= python3
CPPFLAGS += -DRECON_NATIVE_TEST -Iinclude -Inative
CFLAGS += -m32 -std=gnu89 -O0 -g -Wall -Wextra
SOURCES = src/crt.c src/win32.c src/process.c src/compat.c src/entry.c src/decimal.c
HEADERS = $(wildcard include/*.h native/*.h native/sys/*.h)
.PHONY: all check find-check trace-check process-check process-stream-check trace-allocation-check adoption-check clean
all: build/check
build:
	mkdir -p $@
build/check: tests/check.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/check.c $(SOURCES) -o $@
check: build/check
	./build/check
build/find-probe: tests/find-probe.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/find-probe.c $(SOURCES) -o $@
find-check: build/find-probe
	$(PYTHON) tests/check-find.py build/find-probe
build/trace-target.o: tests/trace-target.c | build
	$(CC) -m32 -std=gnu89 -O0 -g -finstrument-functions -c $< -o $@
build/trace-pie: build/trace-target.o src/trace.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIE -pie $^ -o $@
build/trace-exec: build/trace-target.o src/trace.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -no-pie $^ -o $@
trace-check: build/trace-pie build/trace-exec
	python3 tests/check-trace.py build/trace-pie build/trace-exec
build/process-target.o: tests/process-target.c $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -finstrument-functions -c $< -o $@
build/process-target: build/process-target.o $(SOURCES) src/trace.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@
process-check: build/process-target
	python3 tests/check-process.py build/process-target
adoption-check:
	$(PYTHON) tests/check-adoption.py $(SOURCES)
process-stream-check:
	$(PYTHON) tests/check-spawn-streams.py
trace-allocation-check:
	$(PYTHON) tests/check-trace-allocation.py
clean:
	rm -rf build
