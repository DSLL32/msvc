PYTHON ?= python3
TOOL = $(PYTHON) scripts/wine-tool.py
TC = ../../../toolchains/1.0-nt
RECON = ../../../lib/recon
.PHONY: prefix all native native-trace native-check native-trace-check check replay clean reference-build reference-audit native-cache-reference capture-native-boundaries-reference capture-direct-reference capture-specialized-reference graph-audit
all: native
include ../../../tooling/graph-audit/graph-audit.mk
GRAPH_AUDIT_FLAGS = --discovery cfg --check-ret --check-references --check-callbacks --check-ownership --check-extents --check-accepted-extents

# Explicit canonical compile/analysis only. No fixture target depends on Wine.
prefix:
	$(TOOL) --init
build:
	mkdir -p $@
build/%.obj: src/%.c $(wildcard src/*.h) $(RECON)/include/recon.h | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /I$(RECON)/include /Fo$@ $<
OBJECTS = build/startup.obj build/heap.obj build/heap-crt.obj build/stubs.obj build/arguments.obj build/driver.obj build/characters.obj build/paths.obj build/open.obj build/lowio.obj build/prepare.obj build/format.obj build/files.obj build/stdio.obj build/process.obj build/lexer.obj build/resources.obj build/resource-types.obj build/resource-data.obj build/identifiers.obj build/tokens.obj build/numbers.obj build/language.obj build/keywords.obj build/atol.obj build/conversion.obj build/resource-flags.obj build/diagnostics.obj build/menu.obj build/strings.obj build/parser-helpers.obj build/resource-writer.obj build/dialog.obj build/style.obj build/dialog-properties.obj build/wide-numbers.obj build/dialog-controls.obj build/control-fields.obj build/control-kinds.obj build/control-generic.obj build/string-tables.obj build/accelerators.obj build/images.obj build/resource-files.obj build/bitmaps.obj build/raw-data.obj build/wide-strings.obj build/dialog-include.obj build/version.obj build/finish.obj build/crt-state.obj build/termination.obj build/flush-all.obj build/temporary-buffer.obj build/fonts.obj build/legacy-images.obj build/scan-input.obj build/kanji-strings.obj build/spawn-search.obj build/unused-helpers.obj
build/ntdll.lib: src/ntdll.def scripts/import-library.py scripts/binary.py | build prefix
	$(PYTHON) scripts/import-library.py
build/rc.exe: $(OBJECTS) build/ntdll.lib Makefile
	$(TOOL) LINK.EXE /nologo /entry:mainCRTStartup /subsystem:console /map:build/rc.map /out:$@ $(OBJECTS) build/ntdll.lib $(TC)/lib/LIBC.LIB
reference-build: build/rc.exe
runtime.json: functions.json scripts/identify-runtime.py scripts/binary.py $(TC)/bin/RC.EXE $(TC)/lib/LIBC.LIB $(TC)/lib/LIBCMT.LIB $(TC)/lib/SETARGV.OBJ
	$(PYTHON) scripts/identify-runtime.py
build/runtime.json: functions.json build/rc.exe scripts/identify-runtime.py scripts/binary.py
	$(PYTHON) scripts/identify-runtime.py build/rc.exe $@
reference-audit: reference-build runtime.json build/runtime.json
	$(MAKE) graph-audit
# Host-only read-only audits of existing canonical artifacts; no Wine dependency.
# Keep the complete tool report, and let actual tool error diagnostics reach stderr.
graph-audit: $(GRAPH_AUDIT_BIN) | build
	$(GRAPH_AUDIT_BIN) --repo ../../.. --project . --profile rc $(GRAPH_AUDIT_FLAGS) >build/graph-audit.json
	$(PYTHON) scripts/audit-data.py
	$(PYTHON) scripts/audit-uncovered.py --graph-report build/graph-audit.json

# Separate native objects instrument application C only. Shared compatibility
# sources and the small fixture entry compile directly at final link, no archive.
NATIVE_CC ?= clang
NATIVE_FLAGS = -m32 -std=gnu89 -fms-extensions -fasm-blocks -fshort-wchar -O0 -g
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -I$(RECON)/include -I$(RECON)/native -Isrc
NATIVE_EXCLUDED = startup heap-crt arguments paths open lowio format stdio process atol crt-state termination flush-all temporary-buffer scan-input spawn-search
NATIVE_NAMES = $(filter-out $(NATIVE_EXCLUDED),$(patsubst build/%.obj,%,$(OBJECTS)))
NATIVE_OBJECTS = $(addprefix build/native/,$(addsuffix .o,$(NATIVE_NAMES)))
NATIVE_TRACE_OBJECTS = $(addprefix build/native-trace/,$(addsuffix .o,$(NATIVE_NAMES)))
NATIVE_HEADERS = $(wildcard src/*.h $(RECON)/include/*.h $(RECON)/native/*.h $(RECON)/native/sys/*.h)
NATIVE_SHIMS = $(filter-out $(RECON)/src/trace.c,$(wildcard $(RECON)/src/*.c))
native: build/native/RC.EXE
native-trace: build/native-trace/RC.EXE
build/native build/native-trace:
	mkdir -p $@
build/native/%.o: src/%.c $(NATIVE_HEADERS) | build/native
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -c $< -o $@
build/native-trace/%.o: src/%.c $(NATIVE_HEADERS) | build/native-trace
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -finstrument-functions -c $< -o $@
build/native/RC.EXE: $(NATIVE_OBJECTS) src/native-main.c $(NATIVE_SHIMS) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_OBJECTS) src/native-main.c $(NATIVE_SHIMS) -o $@
build/native-trace/RC.EXE: $(NATIVE_TRACE_OBJECTS) src/native-main.c $(NATIVE_SHIMS) $(RECON)/src/trace.c $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_TRACE_OBJECTS) src/native-main.c $(NATIVE_SHIMS) $(RECON)/src/trace.c -o $@
native-check: native
	$(PYTHON) scripts/check-native.py
native-trace-check: native-trace
	$(PYTHON) scripts/check-native.py --trace
check replay: native-trace-check

# Projection refresh is an explicit reference-analysis operation; never capture.
native-cache-reference: native-trace reference-audit
	$(PYTHON) scripts/native-traces.py --write
build/capture-boundary.exe: build/capture-boundary.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<
# Explicit destination is mandatory; frozen reference replacement is deliberate.
capture-native-boundaries-reference: build/capture-boundary.exe
	$(PYTHON) scripts/capture-native-boundaries.py $(CAPTURE_OPTIONS)
build/direct-run.exe: build/direct-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<
capture-direct-reference: build/direct-run.exe
	$(PYTHON) scripts/capture-direct.py
# Original-only observers for historical console/PATH semantic evidence.
build/interrupt-run.obj: ../fixtures/rc-interrupt/capture-interrupt.c $(RECON)/include/recon.h | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /I$(RECON)/include /Fo$@ $<
build/remaining-trace.obj: ../fixtures/rc-remaining/trace-original.c $(RECON)/include/recon.h | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /I$(RECON)/include /Fo$@ $<
build/interrupt-run.exe: build/interrupt-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<
build/remaining-trace.exe: build/remaining-trace.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<
capture-specialized-reference: build/interrupt-run.exe build/remaining-trace.exe
	$(PYTHON) ../fixtures/rc-interrupt/capture.py
	$(PYTHON) ../fixtures/rc-remaining/capture.py
clean:
	rm -rf build
