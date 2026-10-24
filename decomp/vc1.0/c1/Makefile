PYTHON ?= python3
RECON_TOOL_DIR = $(abspath ../../../tooling/recon-tool)
RECON = ../../../lib/recon
NATIVE_CC = clang
NATIVE_CXX = clang++
NATIVE_CPPFLAGS = -DRECON_NATIVE_TRACE -DRECON_NATIVE_TEST -DRECON_DEFAULT_FMODE=0x8000 -I$(RECON)/include -I$(RECON)/native -Isrc
NATIVE_CFLAGS = -std=gnu89 -fms-extensions -fasm-blocks
BINARY_SOURCES = src/c1.c \
    src/stubs.c \
    src/diagnostics.c \
    src/options.c \
    src/option-data.c \
    src/option-state.c \
    src/packing.c \
    src/memory.c \
    src/hash-table.c \
    src/checksums.c \
    src/characters.c \
    src/arguments.c \
    src/vectors.c \
    src/frontend.c \
    src/initialization.c \
    src/preprocessor.c \
    src/preprocessor-directives.c \
    src/pch.c \
    src/pch-auto.c \
    src/pch-files.c \
    src/pch-relocate.c \
    src/pch-reader.c \
    src/pp-expression.c \
    src/pragma.c \
    src/keywords.c \
    src/floating.c \
    src/intermediate.c \
    src/constants.c \
    src/ir.c \
    src/ir-formats.c \
    src/types.c \
    src/based-pointers.c \
    src/scopes.c \
    src/parameters.c \
    src/builtins.c \
    src/builtin-data.c \
    src/macros.c \
    src/macro-expand.c \
    src/global-ir.c \
    src/global-formats.c \
    src/source-input.c \
    src/lexer.c \
    src/token-data.c \
    src/parser.c \
    src/asm.c \
    src/asm-lexer.c \
    src/asm-mnemonics.c \
    src/asm-instructions.c \
    src/asm-registers.c \
    src/asm-symbols.c \
    src/asm-validation.c \
    src/asm-output.c \
    src/asm-numbers.c \
    src/asm-expressions.c \
    src/asm-parser.c \
    src/asm-grammar.c \
    src/asm-stubs.c \
    src/seh.c \
    src/parser-grammar.c \
    src/declarations.c \
    src/declaration-text.c \
    src/control-ir.c \
    src/control-formats.c \
    src/expression-records.c \
    src/conversions.c \
    src/expressions.c \
    src/calls.c \
    src/literal-names.c \
    src/initializer-records.c \
    src/initializers.c \
    src/debug-info.c \
    src/debug-legacy.c \
    src/browser.c \
    src/browser-stubs.c \
    src/program-database.c \
    src/inline-query.c
BINARY_SUPPORT_SOURCES = src/runtime.c $(addprefix $(RECON)/src/,compat.c crt.c decimal.c entry.c process.c trace.c win32.c)

ORIGINAL_TOOLCHAIN = 1.0-nt
ORIGINAL_SOURCES = $(BINARY_SOURCES) src/runtime.c
ORIGINAL_BUILD = build
ORIGINAL_OUTPUT = $(ORIGINAL_BUILD)/c1.exe
ORIGINAL_CPPFLAGS = /Isrc /I$(RECON)/include
ORIGINAL_CFLAGS = /nologo /c /Od /G3
ORIGINAL_LDFLAGS = /nologo /subsystem:console /map:$(ORIGINAL_BUILD)/c1.map
ORIGINAL_LDLIBS = $(ORIGINAL_TOOLCHAIN_ROOT)/lib/BINMODE.OBJ
ORIGINAL_INCLUDE_DIRS = src $(RECON)/include $(ORIGINAL_TOOLCHAIN_ROOT)/include

RECON_AUDIT_PROFILE = c1
RECON_IMAGE_BASE = 65536
RECON_POLICY = policy-data.json
RECON_AUDIT_FLAGS = -q --discovery cfg --check-ret --check-references --check-ownership --check-extents
RECON_AUDIT_IDENTIFY_RUNTIME = 1
RECON_RUNTIME_OVERRIDE_OBJECTS = $(ORIGINAL_BUILD)/obj/src/runtime.c.obj
RECON_RUNTIME_OVERRIDE_MAP = $(ORIGINAL_BUILD)/c1.map
include $(RECON_TOOL_DIR)/recon-tool.mk

src/asm-grammar.c: src/asm-grammar.y scripts/grammar.py scripts/generate-asm-grammar.py
	$(PYTHON) scripts/generate-asm-grammar.py
src/asm-grammar.h: src/asm-grammar.c
