.DEFAULT_GOAL := native
PYTHON ?= python3
NATIVE_CC ?= gcc
RECON := ../../../lib/recon
NATIVE_FLAGS := -m32 -std=gnu89 -fms-extensions -O0 -g -fno-pie
NATIVE_CPPFLAGS := -DRECON_NATIVE_TEST -I$(RECON)/include -I$(RECON)/native
SOURCES := $(wildcard src/*.c)
HEADERS := $(wildcard src/*.h $(RECON)/include/*.h $(RECON)/native/*.h)
NATIVE_OBJECTS := $(patsubst src/%.c,build/native/%.o,$(SOURCES))
ORIGINAL_OBJECTS := $(patsubst src/%.c,build/original/%.obj,$(SOURCES))
SHIMS := $(wildcard $(RECON)/src/*.c)
TOOL := $(PYTHON) scripts/wine-tool.py

.PHONY: inspect native original fixtures
inspect:
	$(PYTHON) scripts/inventory.py

native: build/native/WLINK
# Explicit Wine action. The scaffold is not an accepted main implementation.
original: build/original/WLINK.EXE
build/native build/original:
	mkdir -p $@
build/native/%.o: src/%.c $(HEADERS) Makefile | build/native
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -finstrument-functions -c $< -o $@
build/native/WLINK: $(NATIVE_OBJECTS) $(HEADERS) $(SHIMS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -no-pie $(NATIVE_OBJECTS) $(SHIMS) -o $@
build/original/%.obj: src/%.c $(HEADERS) Makefile | build/original
	$(TOOL) WCC386 -bt=nt -3r -od -d2 -i=R:\\lib\\recon\\include -fo=$@ $<
build/original/WLINK.EXE build/original/wlink.map &: $(ORIGINAL_OBJECTS) scripts/write-link.py Makefile
	$(PYTHON) scripts/write-link.py $(ORIGINAL_OBJECTS)
	$(TOOL) WLINK @build/original/wlink.lnk
fixtures:
	@echo 'HOLD: help reference and native reconstruction have not been captured.' >&2
	@exit 2
