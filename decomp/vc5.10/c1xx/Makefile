PYTHON ?= python3
include ../../../tooling/graph-audit/graph-audit.mk
RECON = ../../../lib/recon
TOOL = $(PYTHON) scripts/wine-tool.py
NATIVE_CC ?= clang
NATIVE_CXX ?= clang++
CPP_SOURCES = src/pools.cpp src/pch.cpp src/arenas.cpp
SOURCES = src/startup.c src/dllmain.c src/exports.c src/frontend.c src/initializers.c src/output.c src/pdb-context.c src/types.c
HEADERS = src/c1xx.h src/frontend.h src/output.h src/pdb-context.h $(RECON)/include/recon.h
OBJECTS = $(patsubst src/%.c,build/%.obj,$(SOURCES)) $(patsubst src/%.cpp,build/%.obj,$(CPP_SOURCES))
NATIVE_OBJECTS = $(patsubst src/%.c,build/native/%.o,$(SOURCES)) $(patsubst src/%.cpp,build/native/%.o,$(CPP_SOURCES))
NATIVE_SHIMS = $(wildcard $(RECON)/src/*.c)
NATIVE_FLAGS = -m32 -std=gnu89 -fms-extensions -fasm-blocks -Wno-ignored-attributes -O0 -g
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -I$(RECON)/include -I$(RECON)/native

.PHONY: all inspect verify-groundwork native-objects native original fixtures replay audit

all: native

inspect:
	$(PYTHON) scripts/inventory.py

verify-groundwork:
	$(PYTHON) scripts/inventory.py --check cases/pe-inventory.json

original: build/c1n32_l.dll

native-objects: $(NATIVE_OBJECTS)

native: build/native/c1xx-host

build build/native:
	mkdir -p $@

build/%.obj: src/%.c $(HEADERS) scripts/record-build.py | build
	$(PYTHON) scripts/record-build.py --record $@.build.json --artifact $@ -- $(TOOL) CL.EXE /nologo /c /MD /Od /W3 /I$(RECON)/include /Fo$@ $<

build/%.obj: src/%.cpp $(HEADERS) scripts/record-build.py | build
	$(PYTHON) scripts/record-build.py --record $@.build.json --artifact $@ -- $(TOOL) CL.EXE /nologo /c /MD /Od /GR- /GX- /W3 /I$(RECON)/include /Fo$@ $<

# Original-only debugger/host, outside native fixture dependencies.
build/dll-capture.obj: scripts/dll-capture.c scripts/invoke-arguments.h | build
	$(TOOL) CL.EXE /nologo /c /MD /Od /W3 /I$(RECON)/include /Fo$@ $<

build/dll-capture.exe: build/dll-capture.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $< MSVCRT.LIB KERNEL32.LIB

build/c1n32_l.dll build/c1xx.map &: $(OBJECTS) src/c1xx.def Makefile
	$(PYTHON) scripts/record-build.py --link --record build/win32-provenance.json --artifact build/c1n32_l.dll -- $(TOOL) LINK.EXE /nologo /dll /nodefaultlib /entry:_DllMainCRTStartup@12 /def:src/c1xx.def /map:build/c1xx.map /out:build/c1n32_l.dll $(OBJECTS) MSVCRT.LIB KERNEL32.LIB

build/native/%.o: src/%.c $(HEADERS) scripts/record-build.py $(wildcard $(RECON)/include/*.h $(RECON)/native/*.h) | build/native
	$(PYTHON) scripts/record-build.py --native --record $@.build.json --artifact $@ -- $(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -Dmain=c1xx_main -Dscan_internal_switches=initialize_intermediate_files -finstrument-functions -c $< -o $@

build/native/%.o: src/%.cpp $(HEADERS) scripts/record-build.py $(wildcard $(RECON)/include/*.h $(RECON)/native/*.h) | build/native
	$(PYTHON) scripts/record-build.py --native --record $@.build.json --artifact $@ -- $(NATIVE_CXX) $(NATIVE_CPPFLAGS) $(filter-out -std=gnu89,$(NATIVE_FLAGS)) -std=gnu++98 -nostdinc++ -fno-exceptions -fno-rtti -Dmain=c1xx_main -Dscan_internal_switches=initialize_intermediate_files -finstrument-functions -c $< -o $@

build/native/c1xx-host: $(NATIVE_OBJECTS) scripts/native-host.c scripts/invoke-arguments.h $(NATIVE_SHIMS) $(HEADERS) $(wildcard $(RECON)/include/*.h)
	$(PYTHON) scripts/record-build.py --native --link --record build/native/provenance.json --artifact $@ -- $(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -no-pie -Isrc scripts/native-host.c $(NATIVE_OBJECTS) $(NATIVE_SHIMS) -o $@

.DELETE_ON_ERROR:

audit: original native $(GRAPH_AUDIT_BIN)
	$(PYTHON) scripts/audit.py

fixtures replay:
	$(PYTHON) scripts/fixtures.py
