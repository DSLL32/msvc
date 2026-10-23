PYTHON ?= python3
RECON = ../../../lib/recon
TOOL = $(PYTHON) scripts/wine-tool.py
SOURCES = $(filter-out src/debug-run.c,$(wildcard src/*.c))
HEADERS = src/cvpack.h $(RECON)/include/recon.h
OBJECTS = $(patsubst src/%.c,build/%.obj,$(SOURCES))
NATIVE_CC ?= clang
NATIVE_FLAGS = -m32 -std=gnu89 -fms-extensions -fasm-blocks -O0 -g
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -I$(RECON)/include -I$(RECON)/native
NATIVE_HEADERS = $(HEADERS) $(wildcard $(RECON)/native/*.h $(RECON)/include/*.h)
NATIVE_OBJECTS = $(patsubst src/%.c,build/native/%.o,$(SOURCES))
NATIVE_SHIMS = $(wildcard $(RECON)/src/*.c)

.PHONY: all native check audit capture-help clean
all: build/CVPACK.EXE
native: build/native/CVPACK.EXE
check: native
	$(PYTHON) scripts/check-native.py
audit: all native
	$(PYTHON) scripts/audit.py
	$(PYTHON) scripts/check-rejections.py

build build/native:
	mkdir -p $@

build/%.obj: src/%.c $(HEADERS) | build
	$(TOOL) CL.EXE /nologo /c /Od /G3 /I$(RECON)/include /Fo$@ $<

build/debug-run.obj: src/debug-run.c $(RECON)/include/recon.h | build
	$(TOOL) CL.EXE /nologo /c /Od /G3 /I$(RECON)/include /Fo$@ $<

build/files.obj build/stubs.obj build/native/files.o build/native/stubs.o: src/files.h
build/types.obj build/stubs.obj build/native/types.o build/native/stubs.o: src/types.h

build/CVPACK.EXE: $(OBJECTS) Makefile
	$(TOOL) LINK.EXE /nologo /subsystem:console /map:build/cvpack.map /out:$@ $(OBJECTS)

build/debug-run.exe: build/debug-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<

# Explicit original-only analysis probe; excluded from every fixture dependency.
build/sopen-guard-probe.obj: scripts/sopen-guard-probe.c $(RECON)/include/recon.h | build
	$(TOOL) CL.EXE /nologo /c /Od /G3 /I$(RECON)/include /Fo$@ $<

build/native/%.o: src/%.c $(NATIVE_HEADERS) | build/native
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -finstrument-functions -c $< -o $@

build/native/CVPACK.EXE: $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_OBJECTS) $(NATIVE_SHIMS) -o $@

# Explicit opt-in original capture is outside all fixture prerequisites.
capture-help: build/debug-run.exe
	$(PYTHON) scripts/capture-original.py --case help --output $(OUTPUT)

clean:
	rm -rf build
