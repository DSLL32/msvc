PYTHON ?= python3
TOOL = $(PYTHON) scripts/wine-tool.py
TC = ../../../toolchains/1.0-nt
RECON_TOOL_DIR = $(abspath ../../../tooling/recon-tool)
RECON = ../../../lib/recon
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

ORIGINAL_SOURCES = $(BINARY_SOURCES) src/runtime.c
ORIGINAL_BUILD = build
ORIGINAL_OUTPUT = $(ORIGINAL_BUILD)/c1xx.exe
ORIGINAL_CC = $(TOOL) CL.EXE
ORIGINAL_CPPFLAGS = /Isrc /I$(RECON)/include
ORIGINAL_CFLAGS = /nologo /c /Od /G3
ORIGINAL_LINK = $(TOOL) LINK.EXE
ORIGINAL_LDFLAGS = /nologo /subsystem:console /map:$(ORIGINAL_BUILD)/c1xx.map
ORIGINAL_LDLIBS = $(TC)/lib/BINMODE.OBJ
ORIGINAL_INCLUDE_DIRS = src $(RECON)/include $(TC)/include
ORIGINAL_EXTRA_DEPS = scripts/wine-tool.py

RECON_AUDIT_PROFILE = c1xx
RECON_IMAGE_BASE = 65536
RECON_POLICY = policy-data.json
RECON_AUDIT_FLAGS = --discovery cfg --check-ret --check-references --check-callbacks --check-ownership --check-extents
RECON_AUDIT_METADATA = build/graph-metadata.json
RECON_AUDIT_PREREQUISITES = build/runtime.json $(ORIGINAL_OUTPUT)
include $(RECON_TOOL_DIR)/recon-tool.mk

build/parser-grammar.c: scripts/generate-parser.py scripts/grammar.py src/grammar.bnf
	@mkdir -p $(@D)
	$(PYTHON) scripts/generate-parser.py $@

build/lookahead-grammar.c: scripts/generate-lookahead.py scripts/generate-parser.py scripts/grammar.py src/lookahead.bnf
	@mkdir -p $(@D)
	$(PYTHON) scripts/generate-lookahead.py $@

build/runtime.json: $(ORIGINAL_OUTPUT) scripts/identify-runtime.py scripts/binary.py
	$(PYTHON) scripts/identify-runtime.py $(ORIGINAL_OUTPUT) $@

build/graph-metadata.json: $(RECON_REGISTRY) scripts/graph_metadata.py
	@mkdir -p $(@D)
	$(PYTHON) scripts/graph_metadata.py $@

.PHONY: audit-data audit-parser-grammar audit-lookahead-grammar
audit: audit-data audit-parser-grammar audit-lookahead-grammar

audit-data: $(ORIGINAL_OUTPUT) build/runtime.json $(RECON_REGISTRY)
	$(PYTHON) scripts/audit-data.py

audit-parser-grammar: src/grammar.bnf scripts/grammar.py scripts/generate-parser.py scripts/audit-parser-grammar.py
	$(PYTHON) scripts/audit-parser-grammar.py

audit-lookahead-grammar: src/lookahead.bnf scripts/grammar.py scripts/generate-lookahead.py scripts/audit-lookahead-grammar.py
	$(PYTHON) scripts/audit-lookahead-grammar.py
