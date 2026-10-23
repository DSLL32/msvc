PYTHON ?= python3
CASE ?= all
TOOL = $(PYTHON) scripts/wine-tool.py
TC = ../../../toolchains/1.0-nt
RECON = ../../../lib/recon

.PHONY: all clean check replay prefix win32 audit-win32 native native-check native-trace native-trace-check native-adoption-check
all: native-trace

include ../../../tooling/graph-audit/graph-audit.mk
check replay: native-trace-check

# Explicit original-toolchain compilation/audits; never fixture prerequisites.
win32: build/c1.exe build/runtime.json runtime.json
prefix:
	$(PYTHON) scripts/wine-tool.py --init
runtime.json: scripts/identify-runtime.py scripts/binary.py $(TC)/bin/C13232.EXE $(TC)/lib/LIBC.LIB $(TC)/lib/SETARGV.OBJ
	$(PYTHON) scripts/identify-runtime.py
build/runtime.json: build/c1.exe scripts/identify-runtime.py scripts/binary.py
	$(PYTHON) scripts/identify-runtime.py build/c1.exe $@
audit-win32: win32 $(GRAPH_AUDIT_BIN)
	"$(GRAPH_AUDIT_BIN)" --profile c1 --project . --repo ../../.. --discovery cfg --check-ret --check-references --check-ownership --check-extents
	$(PYTHON) scripts/audit-flag-words.py
	$(PYTHON) scripts/audit-text-masks.py
	$(PYTHON) scripts/audit-state.py
	$(PYTHON) scripts/audit-grammar.py
	$(PYTHON) scripts/audit-asm-grammar.py

build:
	mkdir -p $@

build/%.obj: src/%.c $(RECON)/include/recon.h scripts/wine-tool.py scripts/case_paths.py | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /I$(RECON)/include /Fobuild/$*.obj $<

OBJECTS = build/c1.obj build/stubs.obj build/diagnostics.obj build/options.obj build/option-data.obj build/option-state.obj build/packing.obj build/memory.obj build/hash-table.obj build/checksums.obj build/characters.obj build/arguments.obj build/vectors.obj build/frontend.obj build/initialization.obj build/preprocessor.obj build/preprocessor-directives.obj build/pch.obj build/pch-auto.obj build/pch-files.obj build/pch-relocate.obj build/pch-reader.obj build/pp-expression.obj build/pragma.obj build/keywords.obj build/floating.obj build/intermediate.obj build/runtime.obj build/constants.obj build/ir.obj build/ir-formats.obj build/types.obj build/based-pointers.obj build/scopes.obj build/parameters.obj build/builtins.obj build/builtin-data.obj build/macros.obj build/macro-expand.obj build/global-ir.obj build/global-formats.obj build/source-input.obj build/lexer.obj build/token-data.obj build/parser.obj build/asm.obj build/asm-lexer.obj build/asm-mnemonics.obj build/asm-instructions.obj build/asm-registers.obj build/asm-symbols.obj build/asm-validation.obj build/asm-output.obj build/asm-numbers.obj build/asm-expressions.obj build/asm-parser.obj build/asm-grammar.obj build/asm-stubs.obj build/seh.obj build/parser-grammar.obj build/declarations.obj build/declaration-text.obj build/control-ir.obj build/control-formats.obj build/expression-records.obj build/conversions.obj build/expressions.obj build/calls.obj build/literal-names.obj build/initializer-records.obj build/initializers.obj build/debug-info.obj build/debug-legacy.obj build/browser.obj build/browser-stubs.obj build/program-database.obj

build/c1.exe: $(OBJECTS) $(TC)/lib/BINMODE.OBJ Makefile
	$(TOOL) LINK.EXE /nologo /subsystem:console /map:build/c1.map /out:$@ $(OBJECTS) $(TC)/lib/BINMODE.OBJ


clean:
	rm -rf build

# Follow project-local includes transitively without rebuilding unrelated modules.
build/dependencies.mk: scripts/dependencies.py $(wildcard src/*.c) $(wildcard src/*.h) | build
	$(PYTHON) scripts/dependencies.py

ifneq ($(MAKECMDGOALS),clean)
-include build/dependencies.mk
endif

# Native i386 fixtures use the original source and shared Win32/CRT boundary.
NATIVE_CC ?= clang
NATIVE_FLAGS = -m32 -std=gnu89 -fms-extensions -fasm-blocks -O0 -g
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -DRECON_DEFAULT_FMODE=0x8000 -I$(RECON)/include -I$(RECON)/native
NATIVE_HEADERS = $(wildcard src/*.h $(RECON)/include/*.h $(RECON)/native/*.h)
NATIVE_OBJECTS = $(patsubst build/%.obj,build/native/%.o,$(OBJECTS))
NATIVE_SHIMS = $(filter-out $(RECON)/src/trace.c,$(wildcard $(RECON)/src/*.c))
NATIVE_TRACE_OBJECTS = $(patsubst build/native/%,build/native-trace/%,$(NATIVE_OBJECTS))
.PHONY: native native-trace
native: build/native/C13232.EXE
native-trace: build/native-trace/C13232.EXE
build/native build/native-trace:
	mkdir -p $@
build/native/%.o: src/%.c $(NATIVE_HEADERS) Makefile | build/native
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -c $< -o $@
build/native/C13232.EXE: $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_OBJECTS) $(NATIVE_SHIMS) -o $@
build/native-trace/%.o: src/%.c $(NATIVE_HEADERS) Makefile | build/native-trace
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -DRECON_NATIVE_TRACE -finstrument-functions -c $< -o $@
build/native-trace/runtime.o: src/runtime.c $(NATIVE_HEADERS) | build/native-trace
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -c $< -o $@
build/native-trace/C13232.EXE: $(NATIVE_TRACE_OBJECTS) $(NATIVE_SHIMS) $(RECON)/src/trace.c $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_TRACE_OBJECTS) $(NATIVE_SHIMS) $(RECON)/src/trace.c -o $@

NATIVE_CASES = $(if $(filter all,$(CASE)),,$(foreach case,$(CASE),--case $(case)))
native-check: native
	$(PYTHON) scripts/check-native.py $(NATIVE_CASES)
native-trace-check: native-trace
	$(PYTHON) scripts/check-native.py --trace $(NATIVE_CASES)

# Private oracle mutations; no new fixture inputs or original execution.
native-adoption-check: native-trace
	$(PYTHON) scripts/check-native-rejections.py

# Grammar generation consumes only named source and precedence, never the PE.
src/asm-grammar.c: src/asm-grammar.y scripts/generate-asm-grammar.py scripts/grammar.py
	$(PYTHON) scripts/generate-asm-grammar.py
src/asm-grammar.h: src/asm-grammar.c
	@test -f $@
