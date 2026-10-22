PYTHON ?= python3
TOOL = $(PYTHON) scripts/wine-tool.py
TC = ../../../toolchains/1.0-nt
RECON = ../../../lib/recon
HEADERS = $(filter-out src/omf.h src/demangle.h,$(wildcard src/*.h)) $(RECON)/include/recon.h
OBJECTS = $(addprefix build/,$(addsuffix .obj,link demangle demangle-stubs omf omf-stubs checksum editor edit-stubs dump dump-stubs resources codeview debug-symbols map-file definition-file definition-stubs stubs driver files runtime crt-anchors linker repro image symbols external-output section-layout base-relocations arguments options exports export-directives export-resolution export-object library library-output library-options library-stubs import-library import-library-stubs import-archive import-members imports ordering parse-switch usage dos-stub link-pass pass-one pass-two machine i386-relocations archive coff-io object comdat comdat-selection arena modules text-buffer diagnostics argument-stubs files-stubs module-stubs driver-stubs machine-stubs pass-stubs object-stubs option-stubs link-pass-stubs pass-two-stubs))

.PHONY: all clean check prefix replay replay-fresh replay-specialized replay-verify audit native native-check
all: build/original.exe build/reconstructed.exe build/debug-run.exe

prefix:
	$(PYTHON) scripts/wine-tool.py --init

runtime.json: scripts/identify-runtime.py scripts/binary.py functions.json $(TC)/bin/LINK.EXE $(TC)/lib/LIBC.LIB
	$(PYTHON) scripts/identify-runtime.py

build/runtime.json: build/link.exe scripts/identify-runtime.py scripts/binary.py functions.json
	$(PYTHON) scripts/identify-runtime.py build/link.exe $@

build/original.exe build/reconstructed.exe &: build/link.exe build/runtime.json runtime.json functions.json scripts/compare.py scripts/binary.py
	$(PYTHON) scripts/compare.py --mask-only

check: all audit
	$(PYTHON) scripts/compare.py --require-exit

audit: build/runtime.json runtime.json
	$(PYTHON) scripts/audit-graph.py
	$(PYTHON) scripts/audit-callbacks.py
	$(PYTHON) scripts/audit-data.py
	$(PYTHON) scripts/audit-abi.py

replay: all audit
	$(PYTHON) scripts/compare.py --replay --require-exit

replay-fresh: all audit
	$(PYTHON) scripts/replay-frontiers.py --fresh

replay-specialized: all audit
	$(PYTHON) scripts/replay-frontiers.py --include-specialized

replay-verify: all audit
	$(PYTHON) scripts/replay-frontiers.py --verify-fresh --include-specialized

build:
	mkdir -p $@

build/%.obj: src/%.c $(HEADERS) | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /I$(RECON)/include /Fobuild/$*.obj $<

build/link.exe: $(OBJECTS) Makefile
	$(TOOL) LINK.EXE /nologo /subsystem:console /map:build/link.map /out:$@ $(OBJECTS)

build/debug-run.exe: build/debug-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<

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

# Shared behavioral checker is an immutable fixture dependency.
build/gui-check.obj: ../cl/src/gui-check.c | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fo$@ $<

build/gui-check.exe: build/gui-check.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $< USER32.LIB GDI32.LIB

# The fixture host loads the generated DLL and calls its exported stdcall entry.
build/dll-host.obj: ../fixtures/hello-dll/dll-host.c | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fo$@ $<

build/dll-host.exe: build/dll-host.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $< USER32.LIB

build/debug-dll-host.obj: ../fixtures/link-debug-dll/dll-host.c | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fo$@ $<

build/debug-dll-host.exe: build/debug-dll-host.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $< USER32.LIB

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
NATIVE_SHIMS = $(wildcard $(RECON)/src/*.c)

native: build/native/LINK.EXE

build/native:
	mkdir -p $@

build/native/%.o: src/%.c $(NATIVE_HEADERS) | build/native
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -c $< -o $@

build/native/LINK.EXE: $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_OBJECTS) $(NATIVE_SHIMS) -o $@

native-check: native
	$(PYTHON) scripts/check-native.py
