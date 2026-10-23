PYTHON ?= python3
NATIVE_CC ?= clang
RECON = ../../../lib/recon
TOOL = $(PYTHON) scripts/wine-tool.py
SOURCES = src/driver.c src/initializers.c src/host.c
OBJECTS = $(patsubst src/%.c,build/%.obj,$(SOURCES))
NATIVE_OBJECTS = $(patsubst src/%.c,build/native/%.o,$(SOURCES))
HEADERS = $(wildcard src/*.h $(RECON)/include/*.h $(RECON)/native/*.h $(RECON)/native/sys/*.h)
ORIGINAL_HEADERS = $(wildcard src/*.h)
NATIVE_FLAGS = -m32 -std=gnu89 -fms-extensions -fasm-blocks -O0 -g -fno-pie
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -I$(RECON)/include -I$(RECON)/native
NATIVE_APP_FLAGS = -DRECON_NATIVE_TRACE -finstrument-functions
SHIMS = $(wildcard $(RECON)/src/*.c)

.PHONY: all inspect native original audit fixtures check capture-help
all: native

include ../../../tooling/graph-audit/graph-audit.mk

inspect:
	$(PYTHON) scripts/inspect.py

build build/native:
	mkdir -p $@

build/%.obj: src/%.c $(ORIGINAL_HEADERS) | build
	$(TOOL) CL.EXE /nologo /c /MD /Od /Z7 /I$(RECON)/include /Fo$@ $<

build/operator-new-alias.obj: scripts/weak-import.py | build
	$(PYTHON) scripts/weak-import.py $@

build/link.exe: $(OBJECTS) build/operator-new-alias.obj
	$(TOOL) LINK.EXE /nologo /incremental:no /map:build/link.map /out:$@ $(OBJECTS) build/operator-new-alias.obj kernel32.lib

original: build/link.exe build/capture-help.exe

build/capture-help.exe: scripts/capture-help.c | build
	$(TOOL) CL.EXE /nologo /MT /Od /I$(RECON)/include /Febuild/capture-help.exe /Fobuild/capture-help.obj $<

build/native/%.o: src/%.c $(HEADERS) Makefile | build/native
	$(NATIVE_CC) $(NATIVE_FLAGS) $(NATIVE_CPPFLAGS) $(NATIVE_APP_FLAGS) $(if $(filter src/driver.c,$<),-Dmain=link_main) -c $< -o $@

build/native/LINK.EXE: $(NATIVE_OBJECTS) $(SHIMS) $(HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_FLAGS) $(NATIVE_CPPFLAGS) -no-pie $(NATIVE_OBJECTS) $(SHIMS) -o $@

native: build/native/LINK.EXE

build/runtime.json: build/link.exe runtime.json graph.json scripts/runtime-identity.py scripts/binary.py
	$(PYTHON) scripts/runtime-identity.py --rebuilt

audit: build/runtime.json $(GRAPH_AUDIT_BIN)
	$(PYTHON) scripts/audit.py

# Explicit reference capture stays separate from every fixture prerequisite.
capture-help: build/capture-help.exe
	$(PYTHON) scripts/reference.py --mode original

fixtures check: native
	$(PYTHON) scripts/check-native.py
