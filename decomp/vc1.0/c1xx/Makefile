PYTHON ?= python3
TOOL = $(PYTHON) wine-tool.py
TC = ../../../toolchains/1.0-nt
.PHONY: all prefix check replay audit clean
all: build/original.exe build/reconstructed.exe build/debug-run.exe
prefix:
	$(TOOL) --init
build:
	mkdir -p $@
build/%.obj: %.c | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fobuild/$*.obj $<
OBJECTS = build/c1xx.obj build/stubs.obj build/diagnostics.obj build/options.obj build/linkage.obj build/packing.obj build/memory.obj build/checksums.obj build/characters.obj build/arguments.obj build/vectors.obj build/option-data.obj build/frontend.obj build/initialization.obj build/preprocessor.obj build/keywords.obj build/floating.obj build/cpp-context.obj build/inline-vector.obj build/intermediate.obj build/runtime.obj build/constants.obj build/ir.obj build/ir-formats.obj build/types.obj build/scopes.obj build/declarations.obj build/declaration-bind.obj build/symbol-types.obj build/hash-table.obj build/type-attributes.obj build/parameters.obj build/macros.obj build/global-ir.obj build/global-formats.obj build/source-input.obj build/operators.obj build/token-spellings.obj
build/c1xx.exe: $(OBJECTS) $(TC)/lib/BINMODE.OBJ Makefile
	$(TOOL) LINK.EXE /nologo /subsystem:console /map:build/c1xx.map /out:$@ $(OBJECTS) $(TC)/lib/BINMODE.OBJ
runtime.json: identify-runtime.py binary.py $(TC)/bin/C1XX3232.EXE $(TC)/lib/LIBC.LIB $(TC)/lib/SETARGV.OBJ
	$(PYTHON) identify-runtime.py
build/runtime.json: build/c1xx.exe identify-runtime.py binary.py
	$(PYTHON) identify-runtime.py build/c1xx.exe $@
build/original.exe build/reconstructed.exe &: build/c1xx.exe build/runtime.json runtime.json functions.json compare.py binary.py
	$(PYTHON) compare.py --mask-only
build/debug-run.exe: build/debug-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<
check: all audit
	$(PYTHON) compare.py
replay: all
	$(PYTHON) compare.py --replay
audit: build/runtime.json runtime.json
	$(PYTHON) audit-graph.py
	$(PYTHON) audit-data.py
clean:
	rm -rf build

build/dependencies.mk: dependencies.py $(wildcard *.c) $(wildcard *.h) | build
	$(PYTHON) dependencies.py
ifneq ($(MAKECMDGOALS),clean)
-include build/dependencies.mk
endif
