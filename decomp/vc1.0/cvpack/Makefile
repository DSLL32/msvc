PYTHON ?= python3
include ../../../tooling/graph-audit/graph-audit.mk
RECON = ../../../lib/recon
TOOL = $(PYTHON) scripts/wine-tool.py
SOURCES = $(wildcard src/*.c)
HEADERS = $(wildcard src/*.h) $(RECON)/include/recon.h
OBJECTS = $(patsubst src/%.c,build/%.obj,$(SOURCES))
NATIVE_CC ?= clang
NATIVE_FLAGS = -m32 -std=gnu89 -fms-extensions -fasm-blocks -O0 -g
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -I$(RECON)/include -I$(RECON)/native
NATIVE_HEADERS = $(HEADERS) $(wildcard $(RECON)/native/*.h $(RECON)/include/*.h)
NATIVE_OBJECTS = $(patsubst src/%.c,build/native/%.o,$(SOURCES))
NATIVE_SHIMS = $(wildcard $(RECON)/src/*.c)

.PHONY: all native check audit graph-identities capture-help clean
all: build/CVPACK.EXE
native: build/native/CVPACK.EXE
check: native
	$(PYTHON) scripts/check-native.py
audit: all native $(GRAPH_AUDIT_BIN) build/graph-identities.json
	$(PYTHON) scripts/audit.py
	$(PYTHON) scripts/check-rejections.py

# Explicit strict archive-relocation verification; ordinary audits never refresh.
graph-identities: build/graph-identities.json
build/graph-identities.json: build/CVPACK.EXE build/cvpack.map functions.json runtime.json runtime-anchors.json \
  scripts/graph_identities.py scripts/runtime.py scripts/binary.py scripts/state.py scripts/audit.py \
  ../../../toolchains/1.0-nt/lib/LIBC.LIB ../../../toolchains/1.0-nt/bin/CVPACK.EXE
	$(PYTHON) scripts/graph_identities.py --prepare --output $@

build build/native:
	mkdir -p $@

build/%.obj: src/%.c $(HEADERS) | build
	$(TOOL) CL.EXE /nologo /c /Od /G3 /I$(RECON)/include /Fo$@ $<

build/debug-run.obj: scripts/debug-run.c scripts/buffer-observer.h $(RECON)/include/recon.h | build
	$(TOOL) CL.EXE /nologo /c /Od /G3 /I$(RECON)/include /Fo$@ $<

build/files.obj build/stubs.obj build/native/files.o build/native/stubs.o: src/files.h
build/types.obj build/stubs.obj build/native/types.o build/native/stubs.o: src/types.h

build/CVPACK.EXE build/cvpack.map &: $(OBJECTS) Makefile
	$(TOOL) LINK.EXE /nologo /subsystem:console /map:build/cvpack.map /out:build/CVPACK.EXE $(OBJECTS)

build/debug-run.exe: build/debug-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<

# Explicit original-only analysis probe; excluded from every fixture dependency.
build/sopen-guard-probe.obj: scripts/sopen-guard-probe.c $(RECON)/include/recon.h | build
	$(TOOL) CL.EXE /nologo /c /Od /G3 /I$(RECON)/include /Fo$@ $<

build/sopen-pdb-guard-probe.obj: scripts/sopen-pdb-guard-probe.c $(RECON)/include/recon.h | build
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
