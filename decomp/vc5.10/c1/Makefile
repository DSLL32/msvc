PYTHON ?= python3
NATIVE_CC ?= clang
TC = ../../../toolchains/5.10
RECON = ../../../lib/recon
TOOL = $(PYTHON) scripts/wine-tool.py
NATIVE_FLAGS = -m32 -std=gnu89 -fms-extensions -O0 -g -fno-pie -DRECON_NATIVE_TEST -DRECON_NATIVE_ENTRY=c1_native_host -DRECON_NATIVE_ENTRY_TWO_ARGS -Isrc -I$(RECON)/include -I$(RECON)/native
SHIMS = $(filter-out $(RECON)/src/trace.c,$(wildcard $(RECON)/src/*.c))
NATIVE_NAMES = dllmain exports native-crt pass compiler-state initializers buffers
GRAPH_CONFIG ?= functions.json
GRAPH_METADATA ?=
NATIVE_HEADERS = $(wildcard $(RECON)/include/*.h $(RECON)/native/*.h $(RECON)/native/sys/*.h)

.PHONY: all inspect groundwork check-groundwork native original audit fixtures
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
build/native/host.o: scripts/native-host.c $(wildcard src/*.h) $(NATIVE_HEADERS) Makefile | build/native
	$(NATIVE_CC) $(NATIVE_FLAGS) -c $< -o $@
build/native/c1-host: $(addprefix build/native/,$(addsuffix .o,$(NATIVE_NAMES))) build/native/host.o $(SHIMS) $(RECON)/src/trace.c Makefile
	$(NATIVE_CC) $(NATIVE_FLAGS) -no-pie $(filter %.o,$^) $(SHIMS) $(RECON)/src/trace.c -o $@
native: build/native/c1-host

build/%.obj: src/%.c $(wildcard src/*.h) | build
	$(TOOL) CL.EXE /nologo /c /Od /MD /G5 /Isrc /I$(RECON)/include /Fo$@ $<
build/c1n32_l.dll: build/dllmain.obj build/exports.obj build/native-crt.obj build/pass.obj build/compiler-state.obj build/initializers.obj build/buffers.obj scripts/c1.def
	$(TOOL) LINK.EXE /nologo /dll /nodefaultlib /entry:_DllMainCRTStartup@12 /def:scripts/c1.def /map:build/c1.map /out:$@ build/dllmain.obj build/exports.obj build/native-crt.obj build/pass.obj build/compiler-state.obj build/initializers.obj build/buffers.obj $(TC)/lib/MSVCRT.LIB $(TC)/lib/KERNEL32.LIB
build/c1.dll: build/c1n32_l.dll
	cp $< $@

build/dll-capture.obj: scripts/dll-capture.c | build
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
