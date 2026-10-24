PYTHON ?= python3
NATIVE_CC ?= clang
NATIVE_CXX ?= clang++
RECON = ../../../lib/recon
TOOL = $(PYTHON) scripts/wine-tool.py
SOURCES = src/option-stubs.c src/symbols.c src/arguments.c src/reproduction.c src/memory.c src/files.c src/driver-stubs.c src/driver.c src/initializers.c src/full-link-init.c src/comdat.c src/sections.c src/libraries.c src/input.c src/machine.c
CPP_SOURCES = src/link-driver.cpp src/options.cpp src/full-link.cpp src/full-link-stubs.cpp src/section-driver.cpp src/pass-one.cpp src/input-machine.cpp
OBJECTS = $(patsubst src/%.c,build/%.obj,$(SOURCES)) $(patsubst src/%.cpp,build/%.obj,$(CPP_SOURCES))
NATIVE_OBJECTS = $(patsubst src/%.c,build/native/%.o,$(SOURCES)) $(patsubst src/%.cpp,build/native/%.o,$(CPP_SOURCES)) build/native/native-host.o
HEADERS = $(wildcard src/*.h $(RECON)/include/*.h $(RECON)/native/*.h $(RECON)/native/sys/*.h)
ORIGINAL_HEADERS = $(wildcard src/*.h)
NATIVE_FLAGS = -m32 -std=gnu89 -fms-extensions -fasm-blocks -O0 -g -fno-pie
NATIVE_CPPFLAGS = -DRECON_CRT_PROFILE_MSVCRT_510 -DRECON_NATIVE_TEST -I$(RECON)/include -I$(RECON)/native
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

# The POD thiscall caller requires C++; no RTTI, EH or language runtime.
build/%.obj: src/%.cpp $(ORIGINAL_HEADERS) | build
	$(TOOL) CL.EXE /nologo /c /MD /Od /Z7 /GR- /GX- /I$(RECON)/include /Fo$@ $<

build/native/%.o: src/%.cpp $(HEADERS) Makefile | build/native
	$(NATIVE_CXX) $(filter-out -std=gnu89,$(NATIVE_FLAGS)) -std=gnu++98 -nostdinc++ -fno-exceptions -fno-rtti $(NATIVE_CPPFLAGS) $(NATIVE_APP_FLAGS) -c $< -o $@

build/operator-new-alias.obj: scripts/weak-import.py | build
	$(PYTHON) scripts/weak-import.py $@

build/link.exe: $(OBJECTS) build/operator-new-alias.obj build/operator-delete-alias.obj Makefile
	$(TOOL) LINK.EXE /nologo /incremental:no /map:build/link.map /out:$@ $(OBJECTS) build/operator-new-alias.obj build/operator-delete-alias.obj kernel32.lib

original: build/link.exe build/capture-help.exe

build/capture-help.exe: scripts/capture-help.c | build
	$(TOOL) CL.EXE /nologo /MT /Od /I$(RECON)/include /Febuild/capture-help.exe /Fobuild/capture-help.obj $<

build/native/%.o: src/%.c $(HEADERS) Makefile | build/native
	$(NATIVE_CC) $(NATIVE_FLAGS) $(NATIVE_CPPFLAGS) $(NATIVE_APP_FLAGS) $(if $(filter src/driver.c,$<),-Dmain=link_main) -c $< -o $@

build/native/native-host.o: scripts/native-host.c $(HEADERS) Makefile | build/native
	$(NATIVE_CC) $(NATIVE_FLAGS) $(NATIVE_CPPFLAGS) -Isrc -c $< -o $@

build/native/LINK.EXE: $(NATIVE_OBJECTS) $(SHIMS) $(HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_FLAGS) $(NATIVE_CPPFLAGS) -no-pie $(NATIVE_OBJECTS) $(SHIMS) -o $@

native: build/native/LINK.EXE

build/runtime.json: build/link.exe runtime.json graph.json functions.json scripts/runtime-identity.py scripts/binary.py
	$(PYTHON) scripts/runtime-identity.py --rebuilt

audit: build/runtime.json $(GRAPH_AUDIT_BIN)
	$(PYTHON) scripts/audit.py

# Explicit reference capture stays separate from every fixture prerequisite.
capture-help: build/capture-help.exe
	$(PYTHON) scripts/reference.py --mode original

fixtures check: native
	$(PYTHON) scripts/check-native.py

# Explicit original-only ABI/provider probe; never a fixture prerequisite.
build/operator-delete-alias.obj: scripts/weak-import.py | build
	$(PYTHON) scripts/weak-import.py --delete $@

build/delete-probe.obj: scripts/delete-probe.c | build
	$(TOOL) CL.EXE /nologo /c /MD /Od /Z7 /I$(RECON)/include /Fo$@ $<

build/delete-probe.exe: build/delete-probe.obj build/operator-new-alias.obj build/operator-delete-alias.obj
	$(TOOL) LINK.EXE /nologo /incremental:no /out:$@ $^ kernel32.lib

# Explicit original-only case input preparation, never a fixture prerequisite.
build/hello-win32:
	mkdir -p $@
build/hello-win32/hello.obj: cases/hello-win32/source/hello.c | build/hello-win32
	$(TOOL) CL.EXE /nologo /c /O2 /G5 /Fo$@ /Fdbuild/hello-win32/compiler.pdb $<
