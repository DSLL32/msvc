PYTHON ?= python3
TOOL = $(PYTHON) scripts/wine-tool.py
TC = ../../../toolchains/1.0-nt
.PHONY: all prefix check replay audit clean
all: build/direct-run.exe build/remaining-trace.exe build/interrupt-run.exe build/rcpp.exe build/original.exe build/reconstructed.exe build/debug-run.exe
prefix:
	$(TOOL) --init
build:
	mkdir -p $@
build/%.obj: src/%.c $(wildcard src/*.h) | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fobuild/$*.obj $<
OBJECTS = build/startup.obj build/heap.obj build/stubs.obj build/arguments.obj build/driver.obj build/characters.obj build/paths.obj build/open.obj build/lowio.obj build/prepare.obj build/format.obj build/files.obj build/stdio.obj build/process.obj build/lexer.obj build/resources.obj build/resource-types.obj build/resource-data.obj build/identifiers.obj build/tokens.obj build/numbers.obj build/language.obj build/keywords.obj build/atol.obj build/conversion.obj build/resource-flags.obj build/diagnostics.obj build/menu.obj build/strings.obj build/parser-helpers.obj build/resource-writer.obj build/dialog.obj build/style.obj build/dialog-properties.obj build/wide-numbers.obj build/dialog-controls.obj build/control-fields.obj build/control-kinds.obj build/control-generic.obj build/string-tables.obj build/accelerators.obj build/images.obj build/resource-files.obj build/bitmaps.obj build/raw-data.obj build/wide-strings.obj build/dialog-include.obj build/version.obj build/finish.obj build/crt-state.obj build/termination.obj build/flush-all.obj build/temporary-buffer.obj build/fonts.obj build/legacy-images.obj build/scan-input.obj build/kanji-strings.obj build/spawn-search.obj build/unused-helpers.obj
build/ntdll.lib: src/ntdll.def scripts/import-library.py scripts/binary.py | build prefix
	$(PYTHON) scripts/import-library.py
build/rc.exe: $(OBJECTS) build/ntdll.lib Makefile
	$(TOOL) LINK.EXE /nologo /entry:mainCRTStartup /subsystem:console /map:build/rc.map /out:$@ $(OBJECTS) build/ntdll.lib $(TC)/lib/LIBC.LIB
runtime.json: functions.json scripts/identify-runtime.py scripts/binary.py $(TC)/bin/RC.EXE $(TC)/lib/LIBC.LIB
	$(PYTHON) scripts/identify-runtime.py
build/runtime.json: functions.json build/rc.exe scripts/identify-runtime.py scripts/binary.py
	$(PYTHON) scripts/identify-runtime.py build/rc.exe $@
build/original.exe build/reconstructed.exe &: build/rc.exe build/runtime.json runtime.json functions.json scripts/compare.py scripts/binary.py
	$(PYTHON) scripts/compare.py --mask-only
build/debug-run.exe: build/debug-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<
check: all audit
	$(PYTHON) scripts/compare.py --all-cases
replay: all audit
	$(PYTHON) scripts/compare.py --replay
clean:
	rm -rf build

build/rcpp.exe: $(TC)/bin/RCPP.EXE | build
	ln -sf $(abspath $<) $@

audit: build/runtime.json runtime.json
	$(PYTHON) scripts/audit-graph.py
	$(PYTHON) scripts/audit-uncovered.py

# Console-signal debugger is analysis tooling, never part of RC's target link.
build/interrupt-run.obj: ../fixtures/rc-interrupt/capture-interrupt.c | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fobuild/interrupt-run.obj $<
build/interrupt-run.exe: build/interrupt-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<

# Uncovered-code case capture, analysis-only and excluded from RC's link.
build/remaining-trace.obj: ../fixtures/rc-remaining/trace-original.c | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fobuild/remaining-trace.obj $<
build/remaining-trace.exe: build/remaining-trace.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<

# Two stateless direct helper cases; this debugger is outside the RC binary.
build/direct-run.obj: src/direct-run.c | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fobuild/direct-run.obj $<
build/direct-run.exe: build/direct-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<
