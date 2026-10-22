CC ?= gcc
CPPFLAGS += -DRECON_NATIVE_TEST -Iinclude -Inative
CFLAGS += -m32 -std=gnu89 -O0 -g -Wall -Wextra
SOURCES = src/crt.c src/win32.c
HEADERS = $(wildcard include/*.h native/*.h)
.PHONY: all check clean
all: build/check
build:
	mkdir -p $@
build/check: tests/check.c $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/check.c $(SOURCES) -o $@
check: build/check
	./build/check
clean:
	rm -rf build
