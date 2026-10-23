PYTHON ?= python3
include ../../../tooling/graph-audit/graph-audit.mk
RECON = ../../../lib/recon
TOOL = $(PYTHON) scripts/wine-tool.py
NATIVE_CC ?= clang
NATIVE_FLAGS = -m32 -std=gnu89 -fms-extensions -fasm-blocks -O0 -g
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -I$(RECON)/include -I$(RECON)/native -Isrc
SOURCES = $(wildcard src/*.c)
HEADERS = $(wildcard src/*.h)
OBJECTS = $(patsubst src/%.c,build/%.obj,$(SOURCES))
NATIVE_OBJECTS = $(patsubst src/%.c,build/native/%.o,$(SOURCES))
NATIVE_SHIMS = $(wildcard $(RECON)/src/*.c)
NATIVE_HEADERS = $(HEADERS) $(shell rg --files $(RECON)/include $(RECON)/native -g '*.h')
CHECK_CASES = $(patsubst cases/%/case.json,%,$(wildcard cases/*/case.json))

.PHONY: all native check audit audit-native final-check inspect capture-help
all: build/CVPACK.EXE
native: build/native/CVPACK
inspect:
	$(PYTHON) scripts/inventory.py

build build/native:
	mkdir -p $@

build/%.obj: src/%.c $(HEADERS) $(RECON)/include/recon.h Makefile | build
	$(TOOL) CL.EXE /nologo /c /MD /Od /W3 /I$(RECON)/include /Fo$@ $<

build/resources.res: src/cvpack.rc | build
	$(TOOL) RC.EXE /r /fo$@ $<

build/CVPACK.EXE build/cvpack.map &: $(OBJECTS) build/resources.res
	$(TOOL) LINK.EXE /nologo /subsystem:console /map:build/cvpack.map /out:build/CVPACK.EXE $(OBJECTS) build/resources.res

build/native/%.o: src/%.c $(NATIVE_HEADERS) Makefile | build/native
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -finstrument-functions -c $< -o $@

build/native/driver.o: scripts/native-driver.c $(NATIVE_HEADERS) Makefile | build/native
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -c $< -o $@

build/native/CVPACK: $(NATIVE_OBJECTS) build/native/driver.o $(NATIVE_SHIMS) $(NATIVE_HEADERS)
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_OBJECTS) build/native/driver.o $(NATIVE_SHIMS) -o $@

build/debug-run.obj: scripts/debug-run.c | build
	$(TOOL) CL.EXE /nologo /c /MD /Od /W3 /Fo$@ $<

build/debug-run.exe: build/debug-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<

# Original compilation and native-only regression/audit entry points.
audit: all native $(GRAPH_AUDIT_BIN)
	$(PYTHON) scripts/audit.py
audit-native: native $(GRAPH_AUDIT_BIN)
	$(PYTHON) scripts/audit.py
final-check: native $(GRAPH_AUDIT_BIN)
	$(PYTHON) scripts/check-final.py
check: $(addprefix check-,$(CHECK_CASES))
$(addprefix check-,$(CHECK_CASES)): check-%: native
	$(PYTHON) scripts/check-native.py --case $*

# Explicit original-only capture, never a fixture dependency.
capture-help:
	$(PYTHON) scripts/capture-help.py
