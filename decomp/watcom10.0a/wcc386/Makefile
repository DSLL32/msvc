PYTHON ?= python3
NATIVE_CC ?= gcc
RECON = ../../../lib/recon
SOURCES = $(wildcard src/*.c)
HEADERS = $(wildcard src/*.h $(RECON)/include/*.h $(RECON)/native/*.h $(RECON)/native/sys/*.h)
NATIVE_SHIMS = $(wildcard $(RECON)/src/*.c)
NATIVE_OBJECTS = $(patsubst src/%.c,build/native/%.o,$(SOURCES))
OBJECTS = $(patsubst src/%.c,build/%.obj,$(SOURCES))
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -I$(RECON)/include -I$(RECON)/native
NATIVE_FLAGS = -m32 -std=gnu89 -O0 -g -fno-omit-frame-pointer -fno-pie

.PHONY: all inspect native original check help

all: native

inspect:
	$(PYTHON) scripts/inspect.py

native: build/native/WCC386

build/native:
	mkdir -p $@

build/native/%.o: src/%.c $(HEADERS) | build/native
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -finstrument-functions -c $< -o $@

build/native/WCC386: $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -no-pie $(NATIVE_OBJECTS) $(NATIVE_SHIMS) -o $@

original: build/WCC386.EXE build/wcc386.map

build:
	mkdir -p $@

build/%.obj: src/%.c $(HEADERS) | build
	$(PYTHON) scripts/original.py compile $< $@

build/WCC386.EXE build/wcc386.map &: $(OBJECTS) scripts/original.py Makefile
	$(PYTHON) scripts/original.py link $(OBJECTS)

check:
	@echo "$@: blocked pending the shared CF masking and service capture contracts" >&2
	@exit 2

help:
	@echo 'inspect: verify original hashes and embedded CF metadata without executing it'
	@echo 'native: build the named main INT3 stub; no accepted application functions yet'
	@echo 'original: build the Win32 stub with original Watcom and patched WLINK'
	@echo 'check: blocked until CF masking and service capture are approved'
