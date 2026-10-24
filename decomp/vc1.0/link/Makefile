PYTHON ?= python3
TOOL = $(PYTHON) scripts/wine-tool.py
TC = ../../../toolchains/1.0-nt
RECON = ../../../lib/recon
HEADERS = $(filter-out src/omf.h src/demangle.h,$(wildcard src/*.h)) $(RECON)/include/recon.h
OBJECTS = $(addprefix build/,$(addsuffix .obj,link ide comdat-diagnostics demangle demangle-stubs omf omf-debug omf-stubs checksum editor editor-rebase editor-bind edit-stubs dump disassembler disassembly-output opcode-table disassembler-stubs dump-stubs resources resource-legacy codeview debug-symbols map-file definition-file definition-stubs stubs driver files runtime crt-anchors linker repro image symbols external-output section-layout base-relocations arguments options exports export-directives export-resolution export-object library library-output library-options library-stubs import-library import-library-stubs import-archive import-members imports ordering parse-switch usage dos-stub link-pass pass-one pass-two machine i386-relocations m68k-relocations archive coff-io object mac-object mac-debug comdat comdat-selection arena modules mac-sections mac-fixups text-buffer diagnostics argument-stubs files-stubs module-stubs driver-stubs machine-stubs pass-stubs object-stubs option-stubs link-pass-stubs pass-two-stubs))

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
NATIVE_HOST = scripts/native-module.c
NATIVE_HOST_LDFLAGS = -Wl,--wrap=write_import_members -Wl,--wrap=omf_convert_debug_symbols
NATIVE_SHIMS = $(filter-out $(RECON)/src/trace.c,$(wildcard $(RECON)/src/*.c))

native: build/native/LINK.EXE

build/native:
	mkdir -p $@

build/native/%.o: src/%.c $(NATIVE_HEADERS) | build/native
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -c $< -o $@

build/native/LINK.EXE: $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HOST) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HOST) $(NATIVE_HOST_LDFLAGS) -o $@

native-check: native build/native/legacy-diagnostics build/native/ide-protocol build/native/comdat-diagnostics build/native/dispatcher build/native/cvtomf build/native/file-resize build/native/symbol-tables build/native/module-diagnostics build/native/image-checksum build/native/priority-archives build/native/bind-parser build/native/debug-log build/native/rebase-fallback
	$(PYTHON) scripts/check-native.py
	$(PYTHON) scripts/check-legacy-diagnostics.py
	$(PYTHON) scripts/check-ide-protocol.py
	$(PYTHON) scripts/check-comdat-diagnostics.py
	$(PYTHON) scripts/check-dispatcher.py
	$(PYTHON) scripts/check-cvtomf.py
	$(PYTHON) scripts/check-file-resize.py
	$(PYTHON) scripts/check-symbol-tables.py
	$(PYTHON) scripts/check-module-diagnostics.py
	$(PYTHON) scripts/check-image-checksum.py
	$(PYTHON) scripts/check-priority-archives.py
	$(PYTHON) scripts/check-bind-parser.py
	$(PYTHON) scripts/check-debug-log.py
	$(PYTHON) scripts/check-rebase-fallback.py

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

build/native-trace/LINK.EXE: $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HOST) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HOST) $(NATIVE_HOST_LDFLAGS) -o $@

native-trace-check: native-trace build/native-trace/legacy-diagnostics build/native-trace/ide-protocol build/native-trace/comdat-diagnostics build/native-trace/dispatcher build/native-trace/cvtomf build/native-trace/file-resize build/native-trace/symbol-tables build/native-trace/module-diagnostics build/native-trace/image-checksum build/native-trace/priority-archives build/native-trace/bind-parser build/native-trace/debug-log build/native-trace/rebase-fallback
	$(PYTHON) scripts/check-native.py --trace
	$(PYTHON) scripts/check-legacy-diagnostics.py --trace
	$(PYTHON) scripts/check-ide-protocol.py --trace
	$(PYTHON) scripts/check-comdat-diagnostics.py --trace
	$(PYTHON) scripts/check-dispatcher.py --trace
	$(PYTHON) scripts/check-cvtomf.py --trace
	$(PYTHON) scripts/check-file-resize.py --trace
	$(PYTHON) scripts/check-symbol-tables.py --trace
	$(PYTHON) scripts/check-module-diagnostics.py --trace
	$(PYTHON) scripts/check-image-checksum.py --trace
	$(PYTHON) scripts/check-priority-archives.py --trace
	$(PYTHON) scripts/check-bind-parser.py --trace
	$(PYTHON) scripts/check-debug-log.py --trace
	$(PYTHON) scripts/check-rebase-fallback.py --trace

# Explicit original-only analysis helper. Never a native fixture dependency.
.PHONY: capture-helper
capture-helper: prefix
	mkdir -p $(HOME)/.ai-tmp/msvc2/vc1.0/link/tooling
	$(TOOL) CL.EXE /nologo /Od /G3 /I$(RECON)/include '/FeS:\vc1.0\link\tooling\capture-original.exe' '/FoS:\vc1.0\link\tooling\capture-original.obj' src/capture-original.c

# Explicit legacy baseline for audit-adoption diagnostics, never a fixture.
.PHONY: audit-graph-python
audit-graph-python: build/runtime.json runtime.json
	$(PYTHON) scripts/audit-graph.py

# Explicit external Win16 callback test driver; never a fixture dependency.
.PHONY: capture-legacy-diagnostics
capture-legacy-diagnostics: prefix
	mkdir -p $(HOME)/.ai-tmp/msvc2/vc1.0/link/tooling
	$(TOOL) CL.EXE /nologo /Od /G3 /I$(RECON)/include '/FeS:\vc1.0\link\tooling\capture-legacy-diagnostics.exe' '/FoS:\vc1.0\link\tooling\capture-legacy-diagnostics.obj' src/capture-legacy-diagnostics.c

# External minimal-code test executables; use the same reconstructed objects.
NATIVE_DIAGNOSTIC_HOST = scripts/native-legacy-diagnostics.c
build/native/legacy-diagnostics: $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HOST) $(NATIVE_DIAGNOSTIC_HOST) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HOST) $(NATIVE_DIAGNOSTIC_HOST) $(NATIVE_HOST_LDFLAGS) -Wl,--wrap=main -o $@
build/native-trace/legacy-diagnostics: $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HOST) $(NATIVE_DIAGNOSTIC_HOST) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HOST) $(NATIVE_DIAGNOSTIC_HOST) $(NATIVE_HOST_LDFLAGS) -Wl,--wrap=main -o $@

# External C callback dependency for explicit original-only IDE unit captures.
# Native fixtures link this same source directly; they never build/load this PE.
.PHONY: capture-ide-dependency
capture-ide-dependency: prefix
	mkdir -p $(HOME)/.ai-tmp/msvc2/vc1.0/link/tooling
	$(TOOL) CL.EXE /nologo /Od /G3 /MT /LD /I$(RECON)/include '/FeS:\vc1.0\link\tooling\ide-probe.dll' '/FoS:\vc1.0\link\tooling\ide-probe.obj' cases/ide-callback-protocol/sources/ide-probe.c

.PHONY: capture-ide-helper
capture-ide-helper: prefix
	mkdir -p $(HOME)/.ai-tmp/msvc2/vc1.0/link/tooling
	$(TOOL) CL.EXE /nologo /Od /G3 /I$(RECON)/include '/FeS:\vc1.0\link\tooling\capture-ide-protocol.exe' '/FoS:\vc1.0\link\tooling\capture-ide-protocol.obj' src/capture-ide-protocol.c

NATIVE_IDE_HOST = scripts/native-ide-protocol.c cases/ide-callback-protocol/sources/ide-probe.c
NATIVE_IDE_HEADERS = cases/ide-callback-protocol/sources/ide-probe.h
build/native/ide-protocol: $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HOST) $(NATIVE_IDE_HOST) $(NATIVE_IDE_HEADERS) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HOST) $(NATIVE_IDE_HOST) $(NATIVE_HOST_LDFLAGS) -Wl,--wrap=main -o $@
build/native-trace/ide-protocol: $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HOST) $(NATIVE_IDE_HOST) $(NATIVE_IDE_HEADERS) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HOST) $(NATIVE_IDE_HOST) $(NATIVE_HOST_LDFLAGS) -Wl,--wrap=main -o $@

.PHONY: capture-comdat-diagnostics
capture-comdat-diagnostics: prefix
	$(PYTHON) -c "from pathlib import Path; import sys; sys.path.insert(0,'scripts'); from case_paths import SCRATCH; (SCRATCH/'vc1.0/link/tooling').mkdir(parents=True,exist_ok=True)"
	$(TOOL) CL.EXE /nologo /Od /G3 /I$(RECON)/include '/FeS:\vc1.0\link\tooling\capture-comdat-diagnostics.exe' '/FoS:\vc1.0\link\tooling\capture-comdat-diagnostics.obj' src/capture-comdat-diagnostics.c

NATIVE_COMDAT_HOST = scripts/native-comdat-diagnostics.c
build/native/comdat-diagnostics: $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HOST) $(NATIVE_COMDAT_HOST) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HOST) $(NATIVE_COMDAT_HOST) $(NATIVE_HOST_LDFLAGS) -Wl,--wrap=main -o $@
build/native-trace/comdat-diagnostics: $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HOST) $(NATIVE_COMDAT_HOST) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HOST) $(NATIVE_COMDAT_HOST) $(NATIVE_HOST_LDFLAGS) -Wl,--wrap=main -o $@

# Explicit original capture only; never a fixture dependency.
.PHONY: capture-dispatcher
capture-dispatcher: prefix
	mkdir -p $(HOME)/.ai-tmp/msvc2/vc1.0/link/tooling
	$(TOOL) CL.EXE /nologo /Od /G3 /I$(RECON)/include '/FeS:\vc1.0\link\tooling\capture-dispatcher.exe' '/FoS:\vc1.0\link\tooling\capture-dispatcher.obj' src/capture-dispatcher.c

NATIVE_DISPATCHER_HOST = scripts/native-dispatcher.c
build/native/dispatcher: $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HOST) $(NATIVE_DISPATCHER_HOST) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HOST) $(NATIVE_DISPATCHER_HOST) $(NATIVE_HOST_LDFLAGS) -Wl,--wrap=main -o $@
build/native-trace/dispatcher: $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HOST) $(NATIVE_DISPATCHER_HOST) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HOST) $(NATIVE_DISPATCHER_HOST) $(NATIVE_HOST_LDFLAGS) -Wl,--wrap=main -o $@

.PHONY: capture-cvtomf
capture-cvtomf: prefix
	mkdir -p $(HOME)/.ai-tmp/msvc2/vc1.0/link/tooling
	$(TOOL) CL.EXE /nologo /Od /G3 /I$(RECON)/include '/FeS:\vc1.0\link\tooling\capture-cvtomf.exe' '/FoS:\vc1.0\link\tooling\capture-cvtomf.obj' src/capture-cvtomf.c

NATIVE_CVTOMF_HOST = scripts/native-cvtomf.c
build/native/cvtomf: $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HOST) $(NATIVE_CVTOMF_HOST) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HOST) $(NATIVE_CVTOMF_HOST) $(NATIVE_HOST_LDFLAGS) -Wl,--wrap=main -o $@
build/native-trace/cvtomf: $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HOST) $(NATIVE_CVTOMF_HOST) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HOST) $(NATIVE_CVTOMF_HOST) $(NATIVE_HOST_LDFLAGS) -Wl,--wrap=main -o $@

build/omf-debug.obj build/native/omf-debug.o build/native-trace/omf-debug.o: src/omf.h

# Explicit original-only custom file-backend lifecycle controller.
.PHONY: capture-file-resize
capture-file-resize: prefix
	mkdir -p $(HOME)/.ai-tmp/msvc2/vc1.0/link/tooling
	$(TOOL) CL.EXE /nologo /Od /G3 /I$(RECON)/include '/FeS:\vc1.0\link\tooling\capture-file-resize.exe' '/FoS:\vc1.0\link\tooling\capture-file-resize.obj' src/capture-file-resize.c

NATIVE_FILE_RESIZE_HOST = scripts/native-file-resize.c
build/native/file-resize: $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HOST) $(NATIVE_FILE_RESIZE_HOST) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HOST) $(NATIVE_FILE_RESIZE_HOST) -DRECON_NATIVE_STARTUP_CONTROLFP_53 $(NATIVE_HOST_LDFLAGS) -Wl,--wrap=main -o $@
build/native-trace/file-resize: $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HOST) $(NATIVE_FILE_RESIZE_HOST) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HOST) $(NATIVE_FILE_RESIZE_HOST) -DRECON_NATIVE_STARTUP_CONTROLFP_53 $(NATIVE_HOST_LDFLAGS) -Wl,--wrap=main -o $@

# Explicit original-only custom symbol-table controller, never a fixture edge.
.PHONY: capture-symbol-tables
capture-symbol-tables: prefix
	mkdir -p $(HOME)/.ai-tmp/msvc2/vc1.0/link/tooling
	$(TOOL) CL.EXE /nologo /Od /G3 /I$(RECON)/include '/FeS:\vc1.0\link\tooling\capture-symbol-tables.exe' '/FoS:\vc1.0\link\tooling\capture-symbol-tables.obj' src/capture-symbol-tables.c

NATIVE_SYMBOL_TABLES_HOST = scripts/native-symbol-tables.c
build/native/symbol-tables: $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HOST) $(NATIVE_SYMBOL_TABLES_HOST) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HOST) $(NATIVE_SYMBOL_TABLES_HOST) -DRECON_NATIVE_STARTUP_CONTROLFP_53 $(NATIVE_HOST_LDFLAGS) -Wl,--wrap=main -o $@
build/native-trace/symbol-tables: $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HOST) $(NATIVE_SYMBOL_TABLES_HOST) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HOST) $(NATIVE_SYMBOL_TABLES_HOST) -DRECON_NATIVE_STARTUP_CONTROLFP_53 $(NATIVE_HOST_LDFLAGS) -Wl,--wrap=main -o $@

# Explicit original-only module/library diagnostic controller, never fixture edge.
.PHONY: capture-module-diagnostics
capture-module-diagnostics: prefix
	mkdir -p $(HOME)/.ai-tmp/msvc2/vc1.0/link/tooling
	$(TOOL) CL.EXE /nologo /Od /G3 /I$(RECON)/include '/FeS:\vc1.0\link\tooling\capture-module-diagnostics.exe' '/FoS:\vc1.0\link\tooling\capture-module-diagnostics.obj' src/capture-module-diagnostics.c

NATIVE_MODULE_DIAGNOSTICS_HOST = scripts/native-module-diagnostics.c
build/native/module-diagnostics: $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HOST) $(NATIVE_MODULE_DIAGNOSTICS_HOST) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HOST) $(NATIVE_MODULE_DIAGNOSTICS_HOST) -DRECON_NATIVE_STARTUP_CONTROLFP_53 $(NATIVE_HOST_LDFLAGS) -Wl,--wrap=main -o $@
build/native-trace/module-diagnostics: $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HOST) $(NATIVE_MODULE_DIAGNOSTICS_HOST) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HOST) $(NATIVE_MODULE_DIAGNOSTICS_HOST) -DRECON_NATIVE_STARTUP_CONTROLFP_53 $(NATIVE_HOST_LDFLAGS) -Wl,--wrap=main -o $@

.PHONY: capture-image-checksum
capture-image-checksum: prefix
	$(PYTHON) -c "from pathlib import Path; import sys; sys.path.insert(0,'scripts'); from case_paths import SCRATCH; (SCRATCH/'vc1.0/link/tooling').mkdir(parents=True,exist_ok=True)"
	$(TOOL) CL.EXE /nologo /Od /G3 /I$(RECON)/include '/FeS:\vc1.0\link\tooling\capture-image-checksum.exe' '/FoS:\vc1.0\link\tooling\capture-image-checksum.obj' src/capture-image-checksum.c

NATIVE_IMAGE_CHECKSUM_HOST = scripts/native-image-checksum.c
build/native/image-checksum: $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HOST) $(NATIVE_IMAGE_CHECKSUM_HOST) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HOST) $(NATIVE_IMAGE_CHECKSUM_HOST) -DRECON_NATIVE_STARTUP_CONTROLFP_53 $(NATIVE_HOST_LDFLAGS) -Wl,--wrap=main -o $@
build/native-trace/image-checksum: $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HOST) $(NATIVE_IMAGE_CHECKSUM_HOST) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HOST) $(NATIVE_IMAGE_CHECKSUM_HOST) -DRECON_NATIVE_STARTUP_CONTROLFP_53 $(NATIVE_HOST_LDFLAGS) -Wl,--wrap=main -o $@

.PHONY: capture-priority-archives
capture-priority-archives: prefix
	$(PYTHON) -c "from pathlib import Path; import sys; sys.path.insert(0,'scripts'); from case_paths import SCRATCH; (SCRATCH/'vc1.0/link/tooling').mkdir(parents=True,exist_ok=True)"
	$(TOOL) CL.EXE /nologo /Od /G3 /I$(RECON)/include '/FeS:\vc1.0\link\tooling\capture-priority-archives.exe' '/FoS:\vc1.0\link\tooling\capture-priority-archives.obj' src/capture-priority-archives.c

NATIVE_PRIORITY_ARCHIVES_HOST = scripts/native-priority-archives.c
build/native/priority-archives: $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HOST) $(NATIVE_PRIORITY_ARCHIVES_HOST) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HOST) $(NATIVE_PRIORITY_ARCHIVES_HOST) -DRECON_NATIVE_STARTUP_CONTROLFP_53 $(NATIVE_HOST_LDFLAGS) -Wl,--wrap=main -o $@
build/native-trace/priority-archives: $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HOST) $(NATIVE_PRIORITY_ARCHIVES_HOST) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HOST) $(NATIVE_PRIORITY_ARCHIVES_HOST) -DRECON_NATIVE_STARTUP_CONTROLFP_53 $(NATIVE_HOST_LDFLAGS) -Wl,--wrap=main -o $@

.PHONY: capture-bind-parser
capture-bind-parser: prefix
	mkdir -p $(HOME)/.ai-tmp/msvc2/vc1.0/link/tooling
	$(TOOL) CL.EXE /nologo /Od /G3 /I$(RECON)/include '/FeS:\vc1.0\link\tooling\capture-bind-parser.exe' '/FoS:\vc1.0\link\tooling\capture-bind-parser.obj' src/capture-bind-parser.c

NATIVE_BIND_PARSER_HOST = scripts/native-bind-parser.c
build/native/bind-parser: $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HOST) $(NATIVE_BIND_PARSER_HOST) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HOST) $(NATIVE_BIND_PARSER_HOST) -DRECON_NATIVE_STARTUP_CONTROLFP_53 $(NATIVE_HOST_LDFLAGS) -Wl,--wrap=main -o $@
build/native-trace/bind-parser: $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HOST) $(NATIVE_BIND_PARSER_HOST) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HOST) $(NATIVE_BIND_PARSER_HOST) -DRECON_NATIVE_STARTUP_CONTROLFP_53 $(NATIVE_HOST_LDFLAGS) -Wl,--wrap=main -o $@

.PHONY: capture-debug-log
capture-debug-log: prefix
	$(PYTHON) -c "from pathlib import Path; import sys; sys.path.insert(0,'scripts'); from case_paths import SCRATCH; (SCRATCH/'vc1.0/link/tooling').mkdir(parents=True,exist_ok=True)"
	$(TOOL) CL.EXE /nologo /Od /G3 /I$(RECON)/include '/FeS:\vc1.0\link\tooling\capture-debug-log.exe' '/FoS:\vc1.0\link\tooling\capture-debug-log.obj' src/capture-debug-log.c

NATIVE_DEBUG_LOG_HOST = scripts/native-debug-log.c
build/native/debug-log: $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HOST) $(NATIVE_DEBUG_LOG_HOST) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HOST) $(NATIVE_DEBUG_LOG_HOST) -DRECON_NATIVE_STARTUP_CONTROLFP_53 $(NATIVE_HOST_LDFLAGS) -Wl,--wrap=main -o $@
build/native-trace/debug-log: $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HOST) $(NATIVE_DEBUG_LOG_HOST) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HOST) $(NATIVE_DEBUG_LOG_HOST) -DRECON_NATIVE_STARTUP_CONTROLFP_53 $(NATIVE_HOST_LDFLAGS) -Wl,--wrap=main -o $@

.PHONY: capture-rebase-fallback
capture-rebase-fallback: prefix
	$(PYTHON) -c "from pathlib import Path; import sys; sys.path.insert(0,'scripts'); from case_paths import SCRATCH; (SCRATCH/'vc1.0/link/tooling').mkdir(parents=True,exist_ok=True)"
	$(TOOL) CL.EXE /nologo /Od /G3 /I$(RECON)/include '/FeS:\vc1.0\link\tooling\capture-rebase-fallback.exe' '/FoS:\vc1.0\link\tooling\capture-rebase-fallback.obj' src/capture-rebase-fallback.c

NATIVE_REBASE_FALLBACK_HOST = scripts/native-rebase-fallback.c
build/native/rebase-fallback: $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HOST) $(NATIVE_REBASE_FALLBACK_HOST) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HOST) $(NATIVE_REBASE_FALLBACK_HOST) -DRECON_NATIVE_STARTUP_CONTROLFP_53 $(NATIVE_HOST_LDFLAGS) -Wl,--wrap=main -o $@
build/native-trace/rebase-fallback: $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HOST) $(NATIVE_REBASE_FALLBACK_HOST) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_TRACE_OBJECTS) $(NATIVE_TRACE_SHIMS) $(NATIVE_HOST) $(NATIVE_REBASE_FALLBACK_HOST) -DRECON_NATIVE_STARTUP_CONTROLFP_53 $(NATIVE_HOST_LDFLAGS) -Wl,--wrap=main -o $@
