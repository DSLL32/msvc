PYTHON ?= python3
TOOL = $(PYTHON) scripts/wine-tool.py
TC = ../../../toolchains/1.0-nt
RECON_TOOL_DIR = $(abspath ../../../tooling/recon-tool)
include $(RECON_TOOL_DIR)/recon-tool.mk
# Reviewed CRT/startup pin. The module tracer overlay is pinned separately below.
RECON_BASE_COMMIT = e4f17cae9e32249373f16cd2a612dc2b8aa24587
RECON_TRACE_COMMIT = 00875e919af701cc2061fdc90fc34b35dcc6e1a7
RECON = .build/shared-inputs
SHARED_STAMP = $(RECON)/.stamp
NATIVE_CC = clang
NATIVE_CXX = clang++
NATIVE_CPPFLAGS = -DRECON_NATIVE_TRACE -DRECON_NATIVE_TEST -DRECON_NATIVE_STARTUP_CONTROLFP_53 -DRECON_DEFAULT_FMODE=0x8000 -I$(RECON)/include -I$(RECON)/native -Isrc
NATIVE_CFLAGS = -std=gnu89 -fms-extensions -fasm-blocks
BINARY_SOURCES = src/c1xx.c \
    src/stubs.c \
    src/diagnostics.c \
    src/options.c \
    src/linkage.c \
    src/packing.c \
    src/memory.c \
    src/checksums.c \
    src/characters.c \
    src/comments.c \
    src/arguments.c \
    src/vectors.c \
    src/option-data.c \
    src/frontend.c \
    src/initialization.c \
    src/preprocessor.c \
    src/keywords.c \
    src/floating.c \
    src/cpp-context.c \
    src/inline-vector.c \
    src/intermediate.c \
    src/constants.c \
    src/ir.c \
    src/ir-formats.c \
    src/types.c \
    src/scopes.c \
    src/declarations.c \
    src/declaration-bind.c \
    src/declaration-lookup.c \
    src/special-functions.c \
    src/runtime-symbols.c \
    src/option-state.c \
    src/parser-state.c \
    src/resolve-declaration.c \
    src/declarator-shapes.c \
    src/symbol-types.c \
    src/hash-table.c \
    src/type-attributes.c \
    src/parameters.c \
    src/macros.c \
    src/macro-expansion.c \
    src/global-ir.c \
    src/global-formats.c \
    src/source-input.c \
    src/operators.c \
    src/token-spellings.c \
    src/preprocessor-expressions.c \
    src/record-processing.c \
    src/expression-preparation.c \
    src/binary-expressions.c \
    src/condition-expressions.c \
    src/loop-output.c \
    src/constructor-initializers.c \
    src/browser.c \
    src/debug-info.c \
    src/intrinsics.c \
    src/enumerators.c \
    src/braced-initializers.c \
    src/debug-types.c \
    src/debug-fields.c \
    src/object-initializers.c \
    src/text-format.c \
    src/type-diagnostics.c \
    src/directives.c \
    src/literals.c \
    src/virtual-table-output.c \
    src/special-member-lookup.c \
    src/initializer-records.c \
    src/allocation-expressions.c \
    src/call-expressions.c \
    src/numeric-scanning.c \
    src/numeric-conversion.c \
    src/reference-initializers.c \
    src/member-access.c \
    src/member-expressions.c \
    src/member-pointers.c \
    src/mangling.c \
    src/symbol-output.c \
    src/function-finalization.c \
    src/statement-scopes.c \
    src/return-statements.c \
    src/expression-output.c \
    src/expression-records.c \
    src/expression-copy.c \
    src/unary-expressions.c \
    src/inline-analysis.c \
    src/control-ir.c \
    src/control-formats.c \
    src/ir-encoding.c \
    src/function-definitions.c \
    src/frontend-records.c \
    src/identifier-expressions.c \
    build/parser-grammar.c \
    src/parser.c \
    src/declaration-codes.c \
    src/lexer.c \
    src/token-categories.c \
    src/type-specifiers.c \
    src/tags.c \
    src/qualified-names.c \
    src/class-members.c \
    src/inline-capture.c \
    src/type-sizes.c \
    src/arithmetic.c \
    src/lookahead.c \
    src/lookahead-parser.c \
    build/lookahead-grammar.c \
    src/parameter-scopes.c \
    src/storage.c \
    src/class-finalization.c \
    src/class-type-sets.c \
    src/special-members.c
BINARY_SUPPORT_SOURCES = src/runtime.c $(addprefix $(RECON)/src/,compat.c crt.c decimal.c entry.c process.c trace.c win32.c)
.DEFAULT_GOAL := all
include $(RECON_TOOL_DIR)/binary.mk
.PHONY: all native native-trace check replay fixtures cases win32 audit clean prefix unit
all native native-trace: recon-native
$(BINARY_OBJECTS) $(BINARY_SUPPORT_OBJECTS): Makefile | $(SHARED_STAMP)
$(addprefix $(RECON)/src/,compat.c crt.c decimal.c entry.c process.c trace.c win32.c): | $(SHARED_STAMP)
$(SHARED_STAMP): Makefile scripts/materialize-shared.py
	$(PYTHON) scripts/materialize-shared.py $(RECON_BASE_COMMIT) $(RECON_TRACE_COMMIT) $(RECON)
CASE_DIRS := $(sort $(wildcard cases/[0-9][0-9][0-9][0-9]-*))
CASE_TARGETS := $(addsuffix /.build/case.so,$(CASE_DIRS))
cases: recon-native $(CASE_DIRS)
.PHONY: $(CASE_DIRS)
$(CASE_DIRS): | recon-native
	+$(MAKE) -C $@ RECON_SKIP_BINARY_BUILD=1
check replay fixtures: cases $(RECON_TOOL)
	$(RECON_TOOL) run $(CASE_DIRS)
unit: cases $(RECON_TOOL)
	$(RECON_TOOL) test cases/0000-cpp-basic
win32: build/c1xx.exe
prefix:
	$(TOOL) --init
build:
	mkdir -p $@
build/%.obj: src/%.c scripts/wine-tool.py | build prefix $(SHARED_STAMP)
	$(TOOL) CL.EXE /nologo /c /Od /G3 /I$(RECON)/include /Fobuild/$*.obj $<
build/parser-grammar.c: scripts/generate-parser.py scripts/grammar.py src/grammar.bnf | build
	$(PYTHON) scripts/generate-parser.py $@
build/lookahead-grammar.c: scripts/generate-lookahead.py scripts/generate-parser.py scripts/grammar.py src/lookahead.bnf | build
	$(PYTHON) scripts/generate-lookahead.py $@
build/%.obj: build/%.c | prefix $(SHARED_STAMP)
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Isrc /I$(RECON)/include /Fo$@ $<
OBJECTS = build/c1xx.obj build/stubs.obj build/diagnostics.obj build/options.obj build/linkage.obj build/packing.obj build/memory.obj build/checksums.obj build/characters.obj build/comments.obj build/arguments.obj build/vectors.obj build/option-data.obj build/frontend.obj build/initialization.obj build/preprocessor.obj build/keywords.obj build/floating.obj build/cpp-context.obj build/inline-vector.obj build/intermediate.obj build/runtime.obj build/constants.obj build/ir.obj build/ir-formats.obj build/types.obj build/scopes.obj build/declarations.obj build/declaration-bind.obj build/declaration-lookup.obj build/special-functions.obj build/runtime-symbols.obj build/option-state.obj build/parser-state.obj build/resolve-declaration.obj build/declarator-shapes.obj build/symbol-types.obj build/hash-table.obj build/type-attributes.obj build/parameters.obj build/macros.obj build/macro-expansion.obj build/global-ir.obj build/global-formats.obj build/source-input.obj build/operators.obj build/token-spellings.obj build/preprocessor-expressions.obj build/record-processing.obj build/expression-preparation.obj build/binary-expressions.obj build/condition-expressions.obj build/loop-output.obj build/constructor-initializers.obj build/browser.obj build/debug-info.obj build/intrinsics.obj build/enumerators.obj build/braced-initializers.obj build/debug-types.obj build/debug-fields.obj build/object-initializers.obj build/text-format.obj build/type-diagnostics.obj build/directives.obj build/literals.obj build/virtual-table-output.obj build/special-member-lookup.obj build/initializer-records.obj build/allocation-expressions.obj build/call-expressions.obj build/numeric-scanning.obj build/numeric-conversion.obj build/reference-initializers.obj build/member-access.obj build/member-expressions.obj build/member-pointers.obj build/mangling.obj build/symbol-output.obj build/function-finalization.obj build/statement-scopes.obj build/return-statements.obj build/expression-output.obj build/expression-records.obj build/expression-copy.obj build/unary-expressions.obj build/inline-analysis.obj build/control-ir.obj build/control-formats.obj build/ir-encoding.obj build/function-definitions.obj build/frontend-records.obj build/identifier-expressions.obj build/parser-grammar.obj build/parser.obj build/declaration-codes.obj build/lexer.obj build/token-categories.obj build/type-specifiers.obj build/tags.obj build/qualified-names.obj build/class-members.obj build/inline-capture.obj build/type-sizes.obj build/arithmetic.obj build/lookahead.obj build/lookahead-parser.obj build/lookahead-grammar.obj build/parameter-scopes.obj build/storage.obj build/class-finalization.obj build/class-type-sets.obj build/special-members.obj
build/parser.obj: src/parser-declaration-actions.inc src/parser-declarator-actions.inc src/parser-statement-actions.inc src/parser-expression-actions.inc src/parser-allocation-actions.inc
build/c1xx.exe: $(OBJECTS) $(TC)/lib/BINMODE.OBJ Makefile
	$(TOOL) LINK.EXE /nologo /subsystem:console /map:build/c1xx.map /out:$@ $(OBJECTS) $(TC)/lib/BINMODE.OBJ
build/runtime.json: build/c1xx.exe scripts/identify-runtime.py scripts/binary.py
	$(PYTHON) scripts/identify-runtime.py build/c1xx.exe $@
.build/functions.json: binary.c policy-data.json $(RECON_TOOL) | $(firstword $(CASE_DIRS))
	$(RECON_TOOL) registry-export $(firstword $(CASE_DIRS)) --image-base 65536 --policy policy-data.json --output $@
include ../../../tooling/graph-audit/graph-audit.mk
GRAPH_AUDIT_FLAGS = --profile c1xx --project . --repo ../../.. --config .build/functions.json --discovery cfg --check-ret --check-references --check-callbacks --check-ownership --check-extents --metadata build/graph-metadata.json
build/graph-metadata.json: .build/functions.json scripts/graph_metadata.py | build
	$(PYTHON) scripts/graph_metadata.py $@
audit: $(GRAPH_AUDIT_BIN) build/graph-metadata.json build/runtime.json .build/functions.json
	"$(GRAPH_AUDIT_BIN)" $(GRAPH_AUDIT_FLAGS) --report build/graph-audit.json
	$(PYTHON) scripts/audit-data.py
	$(PYTHON) scripts/audit-parser-grammar.py
	$(PYTHON) scripts/audit-lookahead-grammar.py
clean:
	rm -rf .build build

build/dependencies.mk: scripts/dependencies.py $(wildcard src/*.c src/*.h) | build
	$(PYTHON) scripts/dependencies.py
ifneq ($(MAKECMDGOALS),clean)
-include build/dependencies.mk
endif
