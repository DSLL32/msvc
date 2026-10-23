PYTHON ?= python3
NATIVE_CC ?= gcc
RECON ?= ../../../lib/recon
SOURCES = $(wildcard src/*.c)
HEADERS = $(wildcard src/*.h $(RECON)/include/*.h $(RECON)/native/*.h $(RECON)/native/sys/*.h)
NATIVE_SHIMS = $(wildcard $(RECON)/src/*.c)
NATIVE_OBJECTS = $(patsubst src/%.c,build/native/%.o,$(SOURCES))
OBJECTS = $(patsubst src/%.c,build/%.obj,$(SOURCES))
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -I$(RECON)/include -I$(RECON)/native
NATIVE_FLAGS = -m32 -std=gnu89 -O0 -g -fno-omit-frame-pointer -fno-pie

.PHONY: all native original reference-build metadata inspect check fixtures
all: native

native: build/native/WLIB

build/native:
	mkdir -p $@

build/native/%.o: src/%.c $(HEADERS) | build/native
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -finstrument-functions -c $< -o $@

build/native/WLIB: $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -no-pie $(NATIVE_OBJECTS) $(NATIVE_SHIMS) -o $@

original reference-build: build/WLIB.EXE build/wlib.map

build:
	mkdir -p $@

build/%.obj: src/%.c $(HEADERS) scripts/original.py | build
	$(PYTHON) scripts/original.py compile $< $@

build/WLIB.EXE build/wlib.map &: $(OBJECTS) scripts/original.py Makefile
	$(PYTHON) scripts/original.py link $(OBJECTS)

metadata inspect:
	$(PYTHON) scripts/metadata.py

# Metadata verification only. No successful application fixture exists yet.
check:
	$(PYTHON) scripts/metadata.py --check binary.json

fixtures:
	@printf '%s\n' 'HOLD: original help capture is blocked by resource GS clobber; no accepted application functions.' >&2
	@exit 2
