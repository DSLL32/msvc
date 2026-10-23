PYTHON ?= python3
TOOL = $(PYTHON) scripts/wine-tool.py
TC = ../../../toolchains/1.0-nt
RECON = ../../../lib/recon
HEADERS = $(filter-out src/omf.h src/demangle.h,$(wildcard src/*.h)) $(RECON)/include/recon.h
OBJECTS = $(addprefix build/,$(addsuffix .obj,link demangle demangle-stubs omf omf-stubs checksum editor edit-stubs dump disassembler disassembly-output opcode-table disassembler-stubs dump-stubs resources resource-legacy codeview debug-symbols map-file definition-file definition-stubs stubs driver files runtime crt-anchors linker repro image symbols external-output section-layout base-relocations arguments options exports export-directives export-resolution export-object library library-output library-options library-stubs import-library import-library-stubs import-archive import-members imports ordering parse-switch usage dos-stub link-pass pass-one pass-two machine i386-relocations m68k-relocations archive coff-io object mac-object mac-debug comdat comdat-selection arena modules mac-sections mac-fixups text-buffer diagnostics argument-stubs files-stubs module-stubs driver-stubs machine-stubs pass-stubs object-stubs option-stubs link-pass-stubs pass-two-stubs))

.PHONY: all clean check prefix replay audit native native-check native-trace native-trace-check
all: build/link.exe

include ../../../tooling/graph-audit/graph-audit.mk

prefix:
	$(PYTHON) scripts/wine-tool.py --init

runtime.json: scripts/identify-runtime.py scripts/binary.py functions.json $(TC)/bin/LINK.EXE $(TC)/lib/LIBC.LIB
	$(PYTHON) scripts/identify-runtime.py

build/runtime.json: build/link.exe scripts/identify-runtime.py scripts/binary.py functions.json
	$(PYTHON) scripts/identify-runtime.py build/link.exe $@

check replay: native-trace-check

audit: build/runtime.json runtime.json $(GRAPH_AUDIT_BIN)
	$(GRAPH_AUDIT_BIN) --profile link --project . --repo ../../.. --include-evidenced-runtime --check-accepted-extents --check-ret --check-references --check-ownership --check-extents --report build/graph-audit.json
	$(PYTHON) scripts/audit-callbacks.py
	$(PYTHON) scripts/audit-data.py
	$(PYTHON) scripts/audit-abi.py

build:
	mkdir -p $@

build/%.obj: src/%.c $(HEADERS) | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /I$(RECON)/include /Fobuild/$*.obj $<

build/link.exe: $(OBJECTS) Makefile
	$(TOOL) LINK.EXE /nologo /subsystem:console /map:build/link.map /out:$@ $(OBJECTS)

build/linker.obj: src/image.h src/arguments.h src/linker-types.h
build/repro.obj: src/linker-types.h
build/image.obj: src/image.h src/modules.h src/symbols.h src/linker-types.h
build/symbols.obj: src/symbols.h src/linker-types.h
build/arguments.obj: src/arguments.h src/linker-types.h
build/options.obj: src/options.h src/pass-one.h src/image.h src/modules.h src/linker-types.h

clean:
	rm -rf build

build/text-buffer.obj: src/linker-types.h

build/link-pass.obj: src/link-pass.h src/object.h src/options.h src/comdat.h src/modules.h src/image.h src/dos-stub.h

build/modules.obj: src/modules.h src/linker-types.h

build/comdat.obj: src/comdat.h src/modules.h src/linker-types.h

build/arena.obj: src/linker-types.h

build/pass-one.obj: src/pass-one.h src/machine.h src/comdat.h src/image.h src/modules.h src/linker-types.h

build/files.obj: src/files.h src/linker-types.h

build/machine.obj: src/machine.h src/image.h src/modules.h src/arguments.h src/comdat.h src/linker-types.h

build/coff-io.obj: src/linker-types.h

build/object.obj: src/object.h src/pass-one.h src/image.h src/modules.h src/comdat.h src/linker-types.h

build/usage.obj: src/linker-types.h

build/diagnostics.obj: src/linker-types.h

build/argument-stubs.obj: src/arguments.h src/linker-types.h
build/files-stubs.obj: src/files.h src/linker-types.h
build/parse-switch.obj: src/options.h src/image.h src/modules.h src/linker-types.h

build/module-stubs.obj: src/modules.h src/linker-types.h

build/driver-stubs.obj: src/image.h src/modules.h src/symbols.h src/linker-types.h

build/machine-stubs.obj: src/machine.h src/image.h src/modules.h src/linker-types.h

build/pass-stubs.obj: src/pass-one.h src/image.h src/modules.h src/linker-types.h

build/object-stubs.obj: src/object.h src/pass-one.h src/image.h src/modules.h src/comdat.h src/linker-types.h

build/option-stubs.obj: src/options.h src/image.h src/modules.h src/linker-types.h

build/link-pass-stubs.obj: src/link-pass.h src/object.h src/options.h src/comdat.h src/modules.h src/image.h
build/dos-stub.obj: src/dos-stub.h

# Converter-only interfaces do not invalidate unrelated translation units.
build/omf.obj build/omf-stubs.obj: src/omf.h

build/demangle.obj build/demangle-stubs.obj: src/demangle.h

# Linux testing uses the same reconstruction sources. The shim C sources are
# compiled directly at every final link; no native static archive is produced.
NATIVE_CC ?= clang
NATIVE_FLAGS = -m32 -std=gnu89 -fms-extensions -fasm-blocks -O0 -g
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -I$(RECON)/include -I$(RECON)/native
NATIVE_OBJECTS = $(patsubst build/%.obj,build/native/%.o,$(OBJECTS))
NATIVE_HEADERS = $(wildcard src/*.h $(RECON)/include/*.h $(RECON)/native/*.h)
NATIVE_SHIMS = $(filter-out $(RECON)/src/trace.c,$(wildcard $(RECON)/src/*.c))

native: build/native/LINK.EXE

build/native:
	mkdir -p $@

build/native/%.o: src/%.c $(NATIVE_HEADERS) | build/native
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -c $< -o $@

build/native/LINK.EXE: $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_OBJECTS) $(NATIVE_SHIMS) -o $@

native-check: native
	$(PYTHON) scripts/check-native.py

# Application entry tracing is separate from the uninstrumented A/B target.
# The reconstructed CRT override and shared shims remain uninstrumented.
NATIVE_TRACE_OBJECTS = $(patsubst build/native/%,build/native-trace/%,$(NATIVE_OBJECTS))
NATIVE_TRACE_SHIMS = $(NATIVE_SHIMS) $(RECON)/src/trace.c

native-trace: build/native-trace/LINK.EXE

build/native-trace:
	mkdir -p $@

build/native-trace/%.o: src/%.c $(NATIVE_HEADERS) | build/native-trace
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -finstrument-functions -c $< -o $@

build/native-trace/runtime.o: src/runtime.c $(NATIVE_HEADERS) | build/native-trace
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -c $< -o $@

build/native-trace/LINK.EXE: $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) -o $@

native-trace-check: native-trace
	$(PYTHON) scripts/check-native.py --trace

# Explicit original-only analysis helper. Never a native fixture dependency.
.PHONY: capture-helper
capture-helper: prefix
	mkdir -p $(HOME)/.ai-tmp/msvc2/vc1.0/link/tooling
	$(TOOL) CL.EXE /nologo /Od /G3 /I$(RECON)/include '/FeS:\vc1.0\link\tooling\capture-original.exe' '/FoS:\vc1.0\link\tooling\capture-original.obj' src/capture-original.c

# Explicit legacy baseline for audit-adoption diagnostics, never a fixture.
.PHONY: audit-graph-python
audit-graph-python: build/runtime.json runtime.json
	$(PYTHON) scripts/audit-graph.py
