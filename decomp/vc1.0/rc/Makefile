PYTHON ?= python3
TOOL = $(PYTHON) wine-tool.py
TC = ../../../toolchains/1.0-nt
.PHONY: all prefix check replay audit clean
all: build/rcpp.exe build/original.exe build/reconstructed.exe build/debug-run.exe
prefix:
	$(TOOL) --init
build:
	mkdir -p $@
build/%.obj: %.c $(wildcard *.h) | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fobuild/$*.obj $<
OBJECTS = build/startup.obj build/heap.obj build/stubs.obj build/arguments.obj build/driver.obj build/characters.obj build/paths.obj build/open.obj build/lowio.obj build/prepare.obj build/format.obj build/files.obj build/stdio.obj build/process.obj build/lexer.obj build/resources.obj build/resource-types.obj build/resource-data.obj build/identifiers.obj build/tokens.obj build/numbers.obj build/language.obj build/keywords.obj build/atol.obj build/conversion.obj build/resource-flags.obj build/diagnostics.obj build/menu.obj build/strings.obj build/parser-helpers.obj build/resource-writer.obj build/dialog.obj build/style.obj build/dialog-properties.obj build/wide-numbers.obj build/dialog-controls.obj build/control-fields.obj build/control-kinds.obj build/control-generic.obj build/string-tables.obj build/accelerators.obj build/images.obj build/resource-files.obj build/bitmaps.obj build/raw-data.obj build/wide-strings.obj build/dialog-include.obj build/version.obj build/finish.obj build/crt-state.obj build/termination.obj build/flush-all.obj build/temporary-buffer.obj build/fonts.obj build/legacy-images.obj build/scan-input.obj build/kanji-strings.obj
build/ntdll.lib: ntdll.def import-library.py binary.py | build prefix
	$(PYTHON) import-library.py
build/rc.exe: $(OBJECTS) build/ntdll.lib Makefile
	$(TOOL) LINK.EXE /nologo /entry:mainCRTStartup /subsystem:console /map:build/rc.map /out:$@ $(OBJECTS) build/ntdll.lib $(TC)/lib/LIBC.LIB
runtime.json: functions.json identify-runtime.py binary.py $(TC)/bin/RC.EXE $(TC)/lib/LIBC.LIB
	$(PYTHON) identify-runtime.py
build/runtime.json: functions.json build/rc.exe identify-runtime.py binary.py
	$(PYTHON) identify-runtime.py build/rc.exe $@
build/original.exe build/reconstructed.exe &: build/rc.exe build/runtime.json runtime.json functions.json compare.py binary.py
	$(PYTHON) compare.py --mask-only
build/debug-run.exe: build/debug-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<
check: all audit
	$(PYTHON) compare.py --all-cases
replay: all audit
	$(PYTHON) compare.py --replay
clean:
	rm -rf build

build/rcpp.exe: $(TC)/bin/RCPP.EXE | build
	ln -sf $(abspath $<) $@

audit: build/runtime.json runtime.json
	$(PYTHON) audit-graph.py
