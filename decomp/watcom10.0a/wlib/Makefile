PYTHON ?= python3
NATIVE_CC ?= gcc
RECON ?= ../../../lib/recon
SOURCES = $(wildcard src/*.c)
HEADERS = $(wildcard src/*.h $(RECON)/include/*.h $(RECON)/native/*.h $(RECON)/native/sys/*.h)
NATIVE_SHIMS = $(wildcard $(RECON)/src/*.c)
NATIVE_OBJECTS = $(patsubst src/%.c,build/native/%.o,$(SOURCES))
OBJECTS = $(patsubst src/%.c,build/%.obj,$(SOURCES))
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -DRECON_CRT_PROFILE_WATCOM_100A -DRECON_NATIVE_STARTUP_WATCOM_100A_FP -I$(RECON)/include -I$(RECON)/native
NATIVE_FLAGS = -m32 -std=gnu89 -O0 -g -fno-omit-frame-pointer -fno-pie

.PHONY: all native original reference-build metadata inspect check fixtures audit-main-candidate audit-hook-candidate
all: native

native: build/native/WLIB

build/native:
	mkdir -p $@

build/native/%.o: src/%.c $(HEADERS) | build/native
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -finstrument-functions -c $< -o $@

build/native/WLIB: $(NATIVE_OBJECTS) scripts/native-host.c $(NATIVE_SHIMS) $(HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -no-pie $(NATIVE_OBJECTS) scripts/native-host.c $(NATIVE_SHIMS) -o $@

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

# Current accepted prefix; later application callees remain INT3 frontiers.
fixtures: native
	@$(PYTHON) scripts/check-main-prefix.py --hook

include ../../../tooling/graph-audit/graph-audit.mk

# Candidate static checks do not grant runtime reconstruction acceptance.
audit-main-candidate: reference-build $(GRAPH_AUDIT_BIN)
	@$(PYTHON) scripts/audit-main.py "$(GRAPH_AUDIT_BIN)"

# The next candidate's driver pins must replace their pending runtime entries.
audit-hook-candidate: reference-build $(GRAPH_AUDIT_BIN)
	@$(PYTHON) scripts/audit-main.py "$(GRAPH_AUDIT_BIN)" cases/help/hook-frontier/audit-candidate.json build/hook-static-graph.json
