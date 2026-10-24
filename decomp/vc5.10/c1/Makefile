PYTHON ?= python3
NATIVE_CC ?= clang
NATIVE_CXX ?= clang++
TC = ../../../toolchains/5.10
RECON = ../../../lib/recon
TOOL = $(PYTHON) scripts/wine-tool.py
NATIVE_FLAGS = -m32 -std=gnu89 -fms-extensions -O0 -g -fno-pie -DRECON_NATIVE_TEST -DRECON_CRT_PROFILE_MSVCRT_510 -DRECON_NATIVE_ENTRY=c1_native_host -DRECON_NATIVE_ENTRY_TWO_ARGS -Isrc -I$(RECON)/include -I$(RECON)/native
SHIMS = $(filter-out $(RECON)/src/trace.c,$(wildcard $(RECON)/src/*.c))
NATIVE_NAMES = dllmain exports native-crt pass compiler-state initializers buffers pdb pools pch-state main work-buffers options arenas command-line checksums scanner option-matcher
GRAPH_CONFIG ?= functions.json
GRAPH_METADATA ?=
NATIVE_HEADERS = $(wildcard $(RECON)/include/*.h $(RECON)/native/*.h $(RECON)/native/sys/*.h)

.PHONY: all inspect groundwork check-groundwork native native-recorded original audit fixtures
all: native
include ../../../tooling/graph-audit/graph-audit.mk

inspect:
	$(PYTHON) scripts/groundwork.py
groundwork check-groundwork:
	$(PYTHON) scripts/groundwork.py --check cases/inventory.json

build build/native:
	mkdir -p $@
build/native/%.o: src/%.c $(wildcard src/*.h) $(NATIVE_HEADERS) Makefile | build/native
	$(NATIVE_CC) $(NATIVE_FLAGS) -Dmain=c1_main -finstrument-functions -c $< -o $@
build/native/%.o: src/%.cpp $(wildcard src/*.h) $(NATIVE_HEADERS) Makefile | build/native
	$(NATIVE_CXX) $(filter-out -std=gnu89,$(NATIVE_FLAGS)) -std=gnu++98 -nostdinc++ -fno-exceptions -fno-rtti -Dmain=c1_main -finstrument-functions -c $< -o $@
build/native/host.o: scripts/native-host.c scripts/host-vectors.h $(wildcard src/*.h) $(NATIVE_HEADERS) Makefile | build/native
	$(NATIVE_CC) $(NATIVE_FLAGS) -c $< -o $@
build/native/c1-host: $(addprefix build/native/,$(addsuffix .o,$(NATIVE_NAMES))) build/native/host.o $(SHIMS) $(RECON)/src/trace.c Makefile
	$(NATIVE_CC) $(NATIVE_FLAGS) -no-pie $(filter %.o,$^) $(SHIMS) $(RECON)/src/trace.c -o $@
native: build/native/c1-host

native-recorded:
	$(PYTHON) scripts/build-native-recorded.py

build/%.obj: src/%.c $(wildcard src/*.h) | build
	$(TOOL) CL.EXE /nologo /c /Od /MD /G5 /Isrc /I$(RECON)/include /Fo$@ $<
build/%.obj: src/%.cpp $(wildcard src/*.h) | build
	$(TOOL) CL.EXE /nologo /c /Od /MD /G5 /GR- /GX- /Isrc /I$(RECON)/include /Fo$@ $<
build/mspdb50.lib: scripts/mspdb50.def | build
	$(TOOL) LIB.EXE /nologo /def:$< /machine:IX86 /out:$@
build/c1n32_l.dll: build/dllmain.obj build/exports.obj build/native-crt.obj build/pass.obj build/compiler-state.obj build/initializers.obj build/buffers.obj build/pdb.obj build/pools.obj build/pch-state.obj build/main.obj build/work-buffers.obj build/options.obj build/arenas.obj build/command-line.obj build/checksums.obj build/scanner.obj build/option-matcher.obj build/mspdb50.lib scripts/c1.def
	$(TOOL) LINK.EXE /nologo /dll /nodefaultlib /entry:_DllMainCRTStartup@12 /def:scripts/c1.def /map:build/c1.map /out:$@ build/dllmain.obj build/exports.obj build/native-crt.obj build/pass.obj build/compiler-state.obj build/initializers.obj build/buffers.obj build/pdb.obj build/pools.obj build/pch-state.obj build/main.obj build/work-buffers.obj build/options.obj build/arenas.obj build/command-line.obj build/checksums.obj build/scanner.obj build/option-matcher.obj build/mspdb50.lib $(TC)/lib/MSVCRT.LIB $(TC)/lib/KERNEL32.LIB
build/c1.dll: build/c1n32_l.dll
	cp $< $@

build/dll-capture.obj: scripts/dll-capture.c $(wildcard scripts/*-observer.inc) | build
	$(TOOL) CL.EXE /nologo /c /Od /MD /G5 /I$(RECON)/include /Fo$@ $<
build/dll-capture.exe: build/dll-capture.obj
	$(TOOL) LINK.EXE /nologo /out:$@ $< $(TC)/lib/MSVCRT.LIB $(TC)/lib/KERNEL32.LIB
original: build/c1.dll build/dll-capture.exe

audit: build/c1.dll $(GRAPH_AUDIT_BIN)
	"$(GRAPH_AUDIT_BIN)" --profile cl --discovery cfg --repo "$(abspath ../../..)" --project "$(CURDIR)" --original "$(abspath $(TC)/bin/C1.DLL)" --rebuilt "$(CURDIR)/build/c1.dll" --map "$(CURDIR)/build/c1.map" --config "$(CURDIR)/$(GRAPH_CONFIG)" --runtime "$(CURDIR)/runtime.json" --rebuilt-runtime "$(CURDIR)/runtime-rebuilt.json" $(if $(GRAPH_METADATA),--metadata "$(CURDIR)/$(GRAPH_METADATA)") --check-imports --check-ret --check-ownership --check-extents

fixtures: native
	$(PYTHON) scripts/fixtures.py

# Explicit original-only fresh CL-to-C1 contract capture helper.
build/minimal-cl-probe.obj: scripts/minimal-cl-probe.c | build
	$(TOOL) CL.EXE /nologo /c /Od /MD /G5 /Fobuild/minimal-cl-probe.obj $<
build/minimal-cl-probe.exe: build/minimal-cl-probe.obj
	$(TOOL) LINK.EXE /nologo /out:$@ $< $(TC)/lib/MSVCRT.LIB $(TC)/lib/KERNEL32.LIB
