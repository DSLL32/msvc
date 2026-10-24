PYTHON ?= python3
CASE ?= all
TOOL = $(PYTHON) scripts/wine-tool.py
TC = ../../../toolchains/1.0-nt
RECON = ../../../lib/recon

.PHONY: all clean check replay prefix win32 audit-win32 native native-check native-trace native-trace-check native-adoption-check native-direct-check
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

OBJECTS += build/inline-query.obj

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
native-direct-check: native native-trace native-expression-tests native-pdb-tests native-pdb-trace-tests native-resource-tests
	$(PYTHON) scripts/check-direct.py
	$(PYTHON) scripts/check-expression-tests.py
	$(PYTHON) scripts/check-pdb-objects.py
	$(PYTHON) scripts/check-support.py
	$(PYTHON) scripts/check-container.py
	$(PYTHON) scripts/check-pdb-queries.py
	$(PYTHON) scripts/check-resource-tests.py

native-check: native native-trace native-expression-tests native-pdb-tests native-pdb-trace-tests native-resource-tests
	$(PYTHON) scripts/check-native.py $(NATIVE_CASES)
	$(if $(filter all,$(CASE)),$(PYTHON) scripts/check-direct.py,@true)
	$(if $(filter all,$(CASE)),$(PYTHON) scripts/check-expression-tests.py,@true)
	$(if $(filter all,$(CASE)),$(PYTHON) scripts/check-pdb-objects.py,@true)
	$(if $(filter all,$(CASE)),$(PYTHON) scripts/check-support.py,@true)
	$(if $(filter all,$(CASE)),$(PYTHON) scripts/check-container.py,@true)
	$(if $(filter all,$(CASE)),$(PYTHON) scripts/check-pdb-queries.py,@true)
	$(if $(filter all,$(CASE)),$(PYTHON) scripts/check-resource-tests.py,@true)
native-trace-check: native native-trace native-expression-tests native-pdb-tests native-pdb-trace-tests native-resource-tests
	$(PYTHON) scripts/check-native.py --trace $(NATIVE_CASES)
	$(if $(filter all,$(CASE)),$(PYTHON) scripts/check-direct.py,@true)
	$(if $(filter all,$(CASE)),$(PYTHON) scripts/check-expression-tests.py,@true)
	$(if $(filter all,$(CASE)),$(PYTHON) scripts/check-pdb-objects.py,@true)
	$(if $(filter all,$(CASE)),$(PYTHON) scripts/check-support.py,@true)
	$(if $(filter all,$(CASE)),$(PYTHON) scripts/check-container.py,@true)
	$(if $(filter all,$(CASE)),$(PYTHON) scripts/check-pdb-queries.py,@true)
	$(if $(filter all,$(CASE)),$(PYTHON) scripts/check-resource-tests.py,@true)

# Private oracle mutations; no new fixture inputs or original execution.
native-adoption-check: native native-trace native-expression-tests native-pdb-tests native-quickc-tests
	$(PYTHON) scripts/check-native-rejections.py
	$(PYTHON) scripts/check-direct-rejections.py
	$(PYTHON) scripts/check-frontier-abi.py
	$(PYTHON) scripts/check-expression-rejections.py
	$(PYTHON) scripts/check-pdb-cleanup.py
	$(PYTHON) scripts/check-pdb-object-rejections.py
	$(PYTHON) scripts/check-quickc-guard.py

# Grammar generation consumes only named source and precedence, never the PE.
src/asm-grammar.c: src/asm-grammar.y scripts/generate-asm-grammar.py scripts/grammar.py
	$(PYTHON) scripts/generate-asm-grammar.py
src/asm-grammar.h: src/asm-grammar.c
	@test -f $@

# External direct unit instrumentation is outside the reconstructed graph.
.PHONY: native-pdb-tests
native-pdb-tests: build/pdb-tests/C13232.EXE
build/pdb-tests:
	mkdir -p $@
build/pdb-tests/external.o: cases/legacy-pdb-object/external-native.c cases/legacy-pdb-object/external.h cases/pdb-record-interface/external-native.h $(NATIVE_HEADERS) Makefile | build/pdb-tests
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -pthread -c $< -o $@
build/pdb-tests/C13232.EXE: $(NATIVE_OBJECTS) build/pdb-tests/external.o $(NATIVE_SHIMS) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -pthread -rdynamic $(NATIVE_OBJECTS) build/pdb-tests/external.o $(NATIVE_SHIMS) -Wl,--wrap=main,--wrap=open_program_database,--wrap=malloc -ldl -o $@

.PHONY: native-pdb-trace-tests
native-pdb-trace-tests: build/pdb-tests-trace/C13232.EXE
build/pdb-tests-trace:
	mkdir -p $@
build/pdb-tests-trace/C13232.EXE: $(NATIVE_TRACE_OBJECTS) build/pdb-tests/external.o $(NATIVE_SHIMS) $(RECON)/src/trace.c $(NATIVE_HEADERS) Makefile | build/pdb-tests-trace
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -pthread -rdynamic $(NATIVE_TRACE_OBJECTS) build/pdb-tests/external.o $(NATIVE_SHIMS) $(RECON)/src/trace.c -Wl,--wrap=main,--wrap=open_program_database,--wrap=malloc -ldl -o $@

.PHONY: native-expression-tests
native-expression-tests: build/expression-tests/C13232.EXE
build/expression-tests:
	mkdir -p $@
build/expression-tests/external.o: cases/expression-templates/external-native.c cases/expression-templates/external.h $(NATIVE_HEADERS) Makefile | build/expression-tests
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -c $< -o $@
build/expression-tests/C13232.EXE: $(NATIVE_OBJECTS) build/expression-tests/external.o $(NATIVE_SHIMS) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_OBJECTS) build/expression-tests/external.o $(NATIVE_SHIMS) -Wl,--wrap=main,--wrap=finish_declarations -o $@

# A precise guard for the original builtin helper's undefined default path.
.PHONY: native-quickc-tests
native-quickc-tests: build/quickc-tests/C13232.EXE
build/quickc-tests:
	mkdir -p $@
build/quickc-tests/guard.o: cases/quickc-regions/native-guard.c $(NATIVE_HEADERS) Makefile | build/quickc-tests
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -c $< -o $@
build/quickc-tests/C13232.EXE: $(NATIVE_OBJECTS) build/quickc-tests/guard.o $(NATIVE_SHIMS) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_OBJECTS) build/quickc-tests/guard.o $(NATIVE_SHIMS) -Wl,--wrap=main -o $@

# Fixed direct units borrow the actual missing-include owner stack.
.PHONY: native-resource-tests
native-resource-tests: build/resource-tests/C13232.EXE build/resource-tests-trace/C13232.EXE
build/resource-tests build/resource-tests-trace:
	mkdir -p $@
build/resource-tests/external.o: cases/nested-input-resources/external-native.c $(NATIVE_HEADERS) Makefile | build/resource-tests
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -c $< -o $@
build/resource-tests/C13232.EXE: $(NATIVE_OBJECTS) build/resource-tests/external.o $(NATIVE_SHIMS) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_OBJECTS) build/resource-tests/external.o $(NATIVE_SHIMS) -Wl,--wrap=main,--wrap=report_fatal,--wrap=recon_exit -o $@
build/resource-tests-trace/C13232.EXE: $(NATIVE_TRACE_OBJECTS) build/resource-tests/external.o $(NATIVE_SHIMS) $(RECON)/src/trace.c $(NATIVE_HEADERS) Makefile | build/resource-tests-trace
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_TRACE_OBJECTS) build/resource-tests/external.o $(NATIVE_SHIMS) $(RECON)/src/trace.c -Wl,--wrap=main,--wrap=report_fatal,--wrap=recon_exit -o $@
