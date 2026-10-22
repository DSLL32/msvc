PYTHON ?= python3
CASE ?= minimal
TOOL = $(PYTHON) wine-tool.py
TC = ../../../toolchains/1.0-nt

.PHONY: all clean check prefix replay audit
all: build/original.exe build/reconstructed.exe build/debug-run.exe

prefix:
	$(PYTHON) wine-tool.py --init

runtime.json: identify-runtime.py binary.py $(TC)/bin/C13232.EXE $(TC)/lib/LIBC.LIB $(TC)/lib/SETARGV.OBJ
	$(PYTHON) identify-runtime.py

build/runtime.json: build/c1.exe identify-runtime.py binary.py
	$(PYTHON) identify-runtime.py build/c1.exe $@

build/original.exe build/reconstructed.exe &: build/c1.exe build/runtime.json runtime.json functions.json compare.py binary.py
	$(PYTHON) compare.py --mask-only

check: all audit
	$(PYTHON) compare.py --fixture $(CASE)

audit: build/runtime.json runtime.json
	$(PYTHON) audit-graph.py
	$(PYTHON) audit-state.py
	$(PYTHON) audit-grammar.py

replay: all
	$(PYTHON) compare.py --fixture $(CASE) --replay

build:
	mkdir -p $@

build/%.obj: %.c | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fobuild/$*.obj $<

OBJECTS = build/c1.obj build/stubs.obj build/diagnostics.obj build/options.obj build/option-data.obj build/option-state.obj build/packing.obj build/memory.obj build/hash-table.obj build/checksums.obj build/characters.obj build/arguments.obj build/vectors.obj build/frontend.obj build/initialization.obj build/preprocessor.obj build/preprocessor-directives.obj build/pch.obj build/pp-expression.obj build/pragma.obj build/keywords.obj build/floating.obj build/intermediate.obj build/runtime.obj build/constants.obj build/ir.obj build/ir-formats.obj build/types.obj build/scopes.obj build/parameters.obj build/builtins.obj build/builtin-data.obj build/macros.obj build/macro-expand.obj build/global-ir.obj build/global-formats.obj build/source-input.obj build/lexer.obj build/token-data.obj build/parser.obj build/parser-grammar.obj build/declarations.obj build/control-ir.obj build/control-formats.obj build/expression-records.obj build/conversions.obj build/expressions.obj build/calls.obj build/literal-names.obj

build/c1.exe: $(OBJECTS) $(TC)/lib/BINMODE.OBJ Makefile
	$(TOOL) LINK.EXE /nologo /subsystem:console /map:build/c1.map /out:$@ $(OBJECTS) $(TC)/lib/BINMODE.OBJ

build/debug-run.obj: debug-run.c | build prefix
	$(TOOL) CL.EXE /nologo /c /O1 /G3 /Fobuild/debug-run.obj debug-run.c

build/debug-run.exe: build/debug-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<

clean:
	rm -rf build

# Follow project-local includes transitively without rebuilding unrelated modules.
build/dependencies.mk: dependencies.py $(wildcard *.c) $(wildcard *.h) | build
	$(PYTHON) dependencies.py

ifneq ($(MAKECMDGOALS),clean)
-include build/dependencies.mk
endif
