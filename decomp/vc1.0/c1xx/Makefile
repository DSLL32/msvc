PYTHON ?= python3
TOOL = $(PYTHON) scripts/wine-tool.py
TC = ../../../toolchains/1.0-nt
ifneq ($(MAKECMDGOALS),clean)
include build/shared-inputs.mk
endif
.PHONY: refresh-shared-build all win32 prefix check replay audit graph-negative-check reference-capture native-rejections clean
all: native-trace
include ../../../tooling/graph-audit/graph-audit.mk
GRAPH_AUDIT_FLAGS = --profile c1xx --project . --repo ../../.. --discovery cfg --check-ret --check-references --check-callbacks --check-ownership --check-extents --metadata build/graph-metadata.json
win32: build/c1xx.exe build/debug-run.exe
prefix:
	$(TOOL) --init
build:
	mkdir -p $@
# Pin/tree: cases/shared-build-reference.json; no live shared-source fallback.
# Refresh only after reviewing a committed slice with the shared maintainer.
refresh-shared-build:
	$(if $(strip $(RECON_COMMIT)),,$(error set RECON_COMMIT to a reviewed shared commit))
	$(PYTHON) scripts/shared_build_inputs.py --pin-commit "$(RECON_COMMIT)" --makefile build/shared-inputs.mk
build/shared-inputs.mk: cases/shared-build-reference.json scripts/shared_build_inputs.py | build
	$(PYTHON) scripts/shared_build_inputs.py --makefile $@
build/%.obj: src/%.c scripts/wine-tool.py scripts/case_paths.py | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /I$(RECON)/include /Fobuild/$*.obj $<
build/parser-grammar.c: scripts/generate-parser.py scripts/grammar.py src/grammar.bnf | build
	$(PYTHON) scripts/generate-parser.py $@
build/parser-grammar.obj: build/parser-grammar.c scripts/wine-tool.py scripts/case_paths.py src/parser-grammar.h | prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Isrc /I$(RECON)/include /Fo$@ $<
OBJECTS = build/c1xx.obj build/stubs.obj build/diagnostics.obj build/options.obj build/linkage.obj build/packing.obj build/memory.obj build/checksums.obj build/characters.obj build/comments.obj build/arguments.obj build/vectors.obj build/option-data.obj build/frontend.obj build/initialization.obj build/preprocessor.obj build/keywords.obj build/floating.obj build/cpp-context.obj build/inline-vector.obj build/intermediate.obj build/runtime.obj build/constants.obj build/ir.obj build/ir-formats.obj build/types.obj build/scopes.obj build/declarations.obj build/declaration-bind.obj build/declaration-lookup.obj build/special-functions.obj build/runtime-symbols.obj build/option-state.obj build/parser-state.obj build/resolve-declaration.obj build/declarator-shapes.obj build/symbol-types.obj build/hash-table.obj build/type-attributes.obj build/parameters.obj build/macros.obj build/macro-expansion.obj build/global-ir.obj build/global-formats.obj build/source-input.obj build/operators.obj build/token-spellings.obj
OBJECTS += build/record-processing.obj build/expression-preparation.obj build/binary-expressions.obj
OBJECTS += build/condition-expressions.obj build/loop-output.obj
OBJECTS += build/constructor-initializers.obj
OBJECTS += build/debug-info.obj build/intrinsics.obj build/enumerators.obj build/braced-initializers.obj
OBJECTS += build/debug-types.obj build/debug-fields.obj
OBJECTS += build/object-initializers.obj build/text-format.obj build/type-diagnostics.obj build/directives.obj build/literals.obj build/virtual-table-output.obj build/special-member-lookup.obj build/initializer-records.obj
OBJECTS += build/allocation-expressions.obj build/call-expressions.obj
OBJECTS += build/numeric-scanning.obj build/numeric-conversion.obj build/reference-initializers.obj
OBJECTS += build/member-access.obj build/member-expressions.obj build/member-pointers.obj
OBJECTS += build/mangling.obj build/symbol-output.obj build/function-finalization.obj build/statement-scopes.obj build/return-statements.obj
OBJECTS += build/expression-output.obj build/expression-records.obj build/expression-copy.obj
OBJECTS += build/unary-expressions.obj build/inline-analysis.obj
OBJECTS += build/control-ir.obj build/control-formats.obj build/ir-encoding.obj
OBJECTS += build/function-definitions.obj build/frontend-records.obj build/identifier-expressions.obj
OBJECTS += build/parser-grammar.obj
OBJECTS += build/parser.obj build/declaration-codes.obj
OBJECTS += build/lexer.obj build/token-categories.obj build/type-specifiers.obj build/tags.obj build/qualified-names.obj build/class-members.obj build/inline-capture.obj build/type-sizes.obj build/arithmetic.obj build/lookahead.obj build/lookahead-parser.obj build/lookahead-grammar.obj build/parameter-scopes.obj build/storage.obj build/class-finalization.obj build/class-type-sets.obj build/special-members.obj
build/parser.obj: src/parser-declaration-actions.inc src/parser-declarator-actions.inc src/parser-statement-actions.inc src/parser-expression-actions.inc src/parser-allocation-actions.inc
build/lookahead-grammar.c: scripts/generate-lookahead.py scripts/generate-parser.py scripts/grammar.py src/lookahead.bnf | build
	$(PYTHON) scripts/generate-lookahead.py $@
build/lookahead-grammar.obj: build/lookahead-grammar.c scripts/wine-tool.py scripts/case_paths.py src/lookahead-grammar.h | prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Isrc /I$(RECON)/include /Fo$@ $<
build/c1xx.exe: $(OBJECTS) $(TC)/lib/BINMODE.OBJ Makefile
	$(TOOL) LINK.EXE /nologo /subsystem:console /map:build/c1xx.map /out:$@ $(OBJECTS) $(TC)/lib/BINMODE.OBJ
runtime.json: scripts/identify-runtime.py scripts/binary.py scripts/decimal_variant.py runtime-compatible.json evidence/decimal-library-binding/compatibility.json $(TC)/bin/C1XX3232.EXE $(TC)/lib/LIBC.LIB $(TC)/lib/SETARGV.OBJ
	$(PYTHON) scripts/identify-runtime.py
build/runtime.json: build/c1xx.exe scripts/identify-runtime.py scripts/binary.py
	$(PYTHON) scripts/identify-runtime.py build/c1xx.exe $@
build/reference-decimal-observer.obj: src/reference-decimal-observer.c src/debug-run.c
build/reference-decimal-observer.exe: build/reference-decimal-observer.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<
build/reference-close-observer.obj: src/reference-close-observer.c src/debug-run.c
build/reference-close-observer.exe: build/reference-close-observer.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<
build/reference-stream-observer.obj: src/reference-stream-observer.c src/debug-run.c
build/reference-stream-observer.exe: build/reference-stream-observer.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<
build/reference-realloc-observer.obj: src/reference-realloc-observer.c src/debug-run.c
build/reference-realloc-observer.exe: build/reference-realloc-observer.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<
build/reference-empty-include-observer.obj: src/reference-empty-include-observer.c src/debug-run.c
build/reference-empty-include-observer.exe: build/reference-empty-include-observer.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<
build/reference-string-observer.obj: src/reference-string-observer.c src/debug-run.c
build/reference-string-observer.exe: build/reference-string-observer.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<
build/reference-concat-observer.obj: src/reference-concat-observer.c src/debug-run.c
build/reference-concat-observer.exe: build/reference-concat-observer.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<
build/reference-copy-observer.obj: src/reference-copy-observer.c src/debug-run.c
build/reference-copy-observer.exe: build/reference-copy-observer.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<
build/reference-scope-observer.obj: src/reference-scope-observer.c src/debug-run.c
build/reference-scope-observer.exe: build/reference-scope-observer.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<
build/reference-constructor-observer.obj: src/reference-constructor-observer.c src/debug-run.c
build/reference-constructor-observer.exe: build/reference-constructor-observer.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<
build/reference-arithmetic-observer.obj: src/reference-arithmetic-observer.c src/debug-run.c
build/reference-arithmetic-observer.exe: build/reference-arithmetic-observer.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<
build/reference-capture-driver.exe: build/reference-capture-driver.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<
build/reference-run-phase.exe: build/reference-run-phase.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<
build/debug-run.exe: build/debug-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<
# Ordinary regressions consume only cached original references, natively.
check replay: native-trace-check
reference-capture: build/debug-run.exe
	$(PYTHON) scripts/capture-original.py
native-rejections: native-trace
	$(PYTHON) scripts/check-native-boundary.py
	$(PYTHON) scripts/check-scope-policy.py
	$(PYTHON) scripts/check-multiply-policy.py
	$(PYTHON) scripts/check-include-policy.py
	$(PYTHON) scripts/check-single-function-policy.py
	$(PYTHON) scripts/check-debug-width-policy.py
	$(PYTHON) scripts/check-debug-method-null-policy.py
	$(PYTHON) scripts/check-debug-class-domain-policy.py
	$(PYTHON) scripts/check-debug-base-query-policy.py
	$(PYTHON) scripts/check-overload-count-null-policy.py
	$(PYTHON) scripts/check-debug-method-list-null-policy.py
	$(PYTHON) scripts/check-member-pointer-model-policy.py
build/graph-metadata.json: functions.json scripts/graph_metadata.py | build
	$(PYTHON) scripts/graph_metadata.py $@
graph-negative-check: $(GRAPH_AUDIT_BIN) build/graph-metadata.json build/runtime.json runtime.json
	$(PYTHON) scripts/check-graph-audit.py
audit: $(GRAPH_AUDIT_BIN) build/graph-metadata.json build/runtime.json runtime.json src/grammar.bnf scripts/grammar.py scripts/audit-parser-grammar.py src/lookahead.bnf scripts/audit-lookahead-grammar.py
	"$(GRAPH_AUDIT_BIN)" $(GRAPH_AUDIT_FLAGS) --report build/graph-audit.json
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

# Native fixture builds preserve the Win32 C bodies and individual ABIs.
NATIVE_CC ?= clang
NATIVE_FLAGS = -m32 -std=gnu89 -fms-extensions -fasm-blocks -O0 -g
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -DRECON_NATIVE_STARTUP_CONTROLFP_53 -DRECON_DEFAULT_FMODE=0x8000 -I$(RECON)/include -I$(RECON)/native -Isrc
NATIVE_HEADERS = $(wildcard src/*.h $(RECON)/include/*.h $(RECON)/native/*.h $(RECON)/native/sys/*.h) build/native-inputs.stamp
NATIVE_OBJECTS = $(patsubst build/%.obj,build/native/%.o,$(OBJECTS))
NATIVE_SHIMS = $(filter-out $(RECON)/src/trace.c,$(wildcard $(RECON)/src/*.c))
NATIVE_TRACE_OBJECTS = $(patsubst build/native/%,build/native-trace/%,$(NATIVE_OBJECTS))

.PHONY: native native-trace native-check native-trace-check native-inputs-force
native-inputs-force:
build/native-inputs.stamp: native-inputs-force scripts/native-build-signature.py scripts/shared_build_inputs.py | build
	$(PYTHON) scripts/native-build-signature.py "$(RECON)" "$@" "$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS)"
native: build/native/C1XX3232.EXE
native-trace: build/native-trace/C1XX3232.EXE
build/native build/native-trace:
	mkdir -p $@
build/native/%.o: src/%.c $(NATIVE_HEADERS) | build/native
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -c $< -o $@
build/native-trace/%.o: src/%.c $(NATIVE_HEADERS) | build/native-trace
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -finstrument-functions -c $< -o $@
build/native-trace/runtime.o: src/runtime.c $(NATIVE_HEADERS) | build/native-trace
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -c $< -o $@
build/native/parser-grammar.o: build/parser-grammar.c $(NATIVE_HEADERS) | build/native
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -c $< -o $@
build/native/lookahead-grammar.o: build/lookahead-grammar.c $(NATIVE_HEADERS) | build/native
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -c $< -o $@
build/native-trace/parser-grammar.o: build/parser-grammar.c $(NATIVE_HEADERS) | build/native-trace
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -finstrument-functions -c $< -o $@
build/native-trace/lookahead-grammar.o: build/lookahead-grammar.c $(NATIVE_HEADERS) | build/native-trace
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -finstrument-functions -c $< -o $@
build/native/C1XX3232.EXE: $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_OBJECTS) $(NATIVE_SHIMS) -o $@
build/native-trace/C1XX3232.EXE: $(NATIVE_TRACE_OBJECTS) $(NATIVE_SHIMS) $(RECON)/src/trace.c $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_TRACE_OBJECTS) $(NATIVE_SHIMS) $(RECON)/src/trace.c -o $@

native-check: native
	$(PYTHON) scripts/check-native.py
native-trace-check: native-trace
	$(PYTHON) scripts/check-native.py --trace
