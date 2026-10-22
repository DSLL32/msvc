PYTHON ?= python3
CASE ?= minimal
TOOL = $(PYTHON) scripts/wine-tool.py
TC = ../../../toolchains/1.0-nt

.PHONY: all clean check prefix replay replay-all replay-fresh audit
all: build/original.exe build/reconstructed.exe build/debug-run.exe

prefix:
	$(PYTHON) scripts/wine-tool.py --init

runtime.json: scripts/identify-runtime.py scripts/binary.py $(TC)/bin/C13232.EXE $(TC)/lib/LIBC.LIB $(TC)/lib/SETARGV.OBJ
	$(PYTHON) scripts/identify-runtime.py

build/runtime.json: build/c1.exe scripts/identify-runtime.py scripts/binary.py
	$(PYTHON) scripts/identify-runtime.py build/c1.exe $@

build/original.exe build/reconstructed.exe &: build/c1.exe build/runtime.json runtime.json functions.json scripts/compare.py scripts/binary.py
	$(PYTHON) scripts/compare.py --mask-only

check: all audit
	$(PYTHON) scripts/compare.py --fixture $(CASE)

audit: build/runtime.json runtime.json
	$(PYTHON) scripts/audit-graph.py
	$(PYTHON) scripts/audit-state.py
	$(PYTHON) scripts/audit-grammar.py

replay: all
	$(PYTHON) scripts/compare.py --fixture $(CASE) --replay

replay-all: all
	$(PYTHON) scripts/fast-replay.py --fixture all

replay-fresh: all
	$(PYTHON) scripts/compare.py --fixture $(CASE) --replay --fresh-history

build:
	mkdir -p $@

build/%.obj: src/%.c | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fobuild/$*.obj $<

OBJECTS = build/c1.obj build/stubs.obj build/diagnostics.obj build/options.obj build/option-data.obj build/option-state.obj build/packing.obj build/memory.obj build/hash-table.obj build/checksums.obj build/characters.obj build/arguments.obj build/vectors.obj build/frontend.obj build/initialization.obj build/preprocessor.obj build/preprocessor-directives.obj build/pch.obj build/pch-files.obj build/pch-relocate.obj build/pch-reader.obj build/pp-expression.obj build/pragma.obj build/keywords.obj build/floating.obj build/intermediate.obj build/runtime.obj build/constants.obj build/ir.obj build/ir-formats.obj build/types.obj build/scopes.obj build/parameters.obj build/builtins.obj build/builtin-data.obj build/macros.obj build/macro-expand.obj build/global-ir.obj build/global-formats.obj build/source-input.obj build/lexer.obj build/token-data.obj build/parser.obj build/parser-grammar.obj build/declarations.obj build/control-ir.obj build/control-formats.obj build/expression-records.obj build/conversions.obj build/expressions.obj build/calls.obj build/literal-names.obj build/initializer-records.obj build/initializers.obj build/debug-info.obj build/program-database.obj

build/c1.exe: $(OBJECTS) $(TC)/lib/BINMODE.OBJ Makefile
	$(TOOL) LINK.EXE /nologo /subsystem:console /map:build/c1.map /out:$@ $(OBJECTS) $(TC)/lib/BINMODE.OBJ

build/debug-run.obj: src/debug-run.c | build prefix
	$(TOOL) CL.EXE /nologo /c /O1 /G3 /Fobuild/debug-run.obj src/debug-run.c

build/debug-run.exe: build/debug-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<

clean:
	rm -rf build

# Follow project-local includes transitively without rebuilding unrelated modules.
build/dependencies.mk: scripts/dependencies.py $(wildcard src/*.c) $(wildcard src/*.h) | build
	$(PYTHON) scripts/dependencies.py

ifneq ($(MAKECMDGOALS),clean)
-include build/dependencies.mk
endif
