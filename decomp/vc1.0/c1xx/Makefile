PYTHON ?= python3
TOOL = $(PYTHON) scripts/wine-tool.py
TC = ../../../toolchains/1.0-nt
.PHONY: all prefix check replay replay-fresh replay-verify audit clean
all: build/original.exe build/reconstructed.exe build/debug-run.exe
prefix:
	$(TOOL) --init
build:
	mkdir -p $@
build/%.obj: src/%.c | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fobuild/$*.obj $<
build/parser-grammar.c: scripts/generate-parser.py scripts/grammar.py src/grammar.bnf | build
	$(PYTHON) scripts/generate-parser.py $@
build/parser-grammar.obj: build/parser-grammar.c src/parser-grammar.h | prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Isrc /Fo$@ $<
OBJECTS = build/c1xx.obj build/stubs.obj build/diagnostics.obj build/options.obj build/linkage.obj build/packing.obj build/memory.obj build/checksums.obj build/characters.obj build/arguments.obj build/vectors.obj build/option-data.obj build/frontend.obj build/initialization.obj build/preprocessor.obj build/keywords.obj build/floating.obj build/cpp-context.obj build/inline-vector.obj build/intermediate.obj build/runtime.obj build/constants.obj build/ir.obj build/ir-formats.obj build/types.obj build/scopes.obj build/declarations.obj build/declaration-bind.obj build/declaration-lookup.obj build/special-functions.obj build/runtime-symbols.obj build/option-state.obj build/parser-state.obj build/resolve-declaration.obj build/declarator-shapes.obj build/symbol-types.obj build/hash-table.obj build/type-attributes.obj build/parameters.obj build/macros.obj build/global-ir.obj build/global-formats.obj build/source-input.obj build/operators.obj build/token-spellings.obj
OBJECTS += build/record-processing.obj build/expression-preparation.obj build/binary-expressions.obj
OBJECTS += build/constructor-initializers.obj
OBJECTS += build/object-initializers.obj
OBJECTS += build/numeric-scanning.obj build/numeric-conversion.obj
OBJECTS += build/member-access.obj build/member-expressions.obj
OBJECTS += build/mangling.obj build/symbol-output.obj build/function-finalization.obj build/statement-scopes.obj build/return-statements.obj
OBJECTS += build/expression-output.obj build/expression-records.obj
OBJECTS += build/unary-expressions.obj build/inline-analysis.obj
OBJECTS += build/control-ir.obj build/control-formats.obj build/ir-encoding.obj
OBJECTS += build/function-definitions.obj build/frontend-records.obj build/identifier-expressions.obj
OBJECTS += build/parser-grammar.obj
OBJECTS += build/parser.obj build/declaration-codes.obj
OBJECTS += build/lexer.obj build/token-categories.obj build/type-specifiers.obj build/tags.obj build/qualified-names.obj build/class-members.obj build/inline-capture.obj build/type-sizes.obj build/arithmetic.obj build/lookahead.obj build/lookahead-parser.obj build/lookahead-grammar.obj build/parameter-scopes.obj build/storage.obj build/class-finalization.obj build/class-type-sets.obj build/special-members.obj
build/parser.obj: src/parser-declaration-actions.inc src/parser-declarator-actions.inc src/parser-statement-actions.inc src/parser-expression-actions.inc src/parser-allocation-actions.inc
build/lookahead-grammar.c: scripts/generate-lookahead.py scripts/generate-parser.py scripts/grammar.py src/lookahead.bnf | build
	$(PYTHON) scripts/generate-lookahead.py $@
build/lookahead-grammar.obj: build/lookahead-grammar.c src/lookahead-grammar.h | prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Isrc /Fo$@ $<
build/c1xx.exe: $(OBJECTS) $(TC)/lib/BINMODE.OBJ Makefile
	$(TOOL) LINK.EXE /nologo /subsystem:console /map:build/c1xx.map /out:$@ $(OBJECTS) $(TC)/lib/BINMODE.OBJ
runtime.json: scripts/identify-runtime.py scripts/binary.py $(TC)/bin/C1XX3232.EXE $(TC)/lib/LIBC.LIB $(TC)/lib/SETARGV.OBJ
	$(PYTHON) scripts/identify-runtime.py
build/runtime.json: build/c1xx.exe scripts/identify-runtime.py scripts/binary.py
	$(PYTHON) scripts/identify-runtime.py build/c1xx.exe $@
build/original.exe build/reconstructed.exe &: build/c1xx.exe build/runtime.json runtime.json functions.json scripts/compare.py scripts/binary.py
	$(PYTHON) scripts/compare.py --mask-only
build/debug-run.exe: build/debug-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<
check: all audit
	$(PYTHON) scripts/compare.py
replay: all
	$(PYTHON) scripts/replay-frontiers.py
replay-fresh: all
	$(PYTHON) scripts/replay-frontiers.py --fresh
replay-verify: all
	$(PYTHON) scripts/replay-frontiers.py --verify
audit: build/runtime.json runtime.json src/grammar.bnf scripts/grammar.py scripts/audit-parser-grammar.py src/lookahead.bnf scripts/audit-lookahead-grammar.py
	$(PYTHON) scripts/audit-graph.py
	$(PYTHON) scripts/audit-data.py
	$(PYTHON) scripts/audit-parser-grammar.py
	$(PYTHON) scripts/audit-lookahead-grammar.py
clean:
	rm -rf build

build/dependencies.mk: scripts/dependencies.py $(wildcard src/*.c) $(wildcard src/*.h) | build
	$(PYTHON) scripts/dependencies.py
ifneq ($(MAKECMDGOALS),clean)
-include build/dependencies.mk
endif
