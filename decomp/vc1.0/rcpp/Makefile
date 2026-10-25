RECON_TOOL_DIR = $(abspath ../../../tooling/recon-tool)
RECON = ../../../lib/recon
NATIVE_CC = clang
NATIVE_CXX = clang++
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -DRECON_NATIVE_ENTRY=rc_main -I$(RECON)/include -I$(RECON)/native -Isrc -Icrt
NATIVE_CFLAGS = -std=gnu89 -fms-extensions -fasm-blocks
BINARY_SOURCES = src/main.c \
    src/options.c \
    src/lexer-data.c \
    src/initialize.c \
    src/symbols.c \
    src/input.c \
    src/encoding.c \
    src/line-output.c \
    src/format.c \
    src/line-start.c \
    src/tokens.c \
    src/directives.c \
    src/scanning.c \
    src/directive-names.c \
    src/lexer.c \
    src/numbers.c \
    src/constants.c \
    src/strings.c \
    src/includes.c \
    src/comments.c \
    src/macro-definition.c \
    src/condition.c \
    src/macro-expansion.c \
    src/macro-identifiers.c \
    src/macro-arguments.c \
    src/characters.c \
    src/messages.c \
    src/diagnostics.c \
    crt/legacy-ctype.c
BINARY_SUPPORT_SOURCES = crt/file-runtime.c $(wildcard $(RECON)/src/*.c)

ORIGINAL_TOOLCHAIN = 1.0-nt
ORIGINAL_SOURCES = crt/startup.c \
    crt/stubs.c \
    crt/runtime.c \
    crt/arguments.c \
    src/main.c \
    src/options.c \
    src/lexer-data.c \
    src/initialize.c \
    src/symbols.c \
    crt/file-runtime.c \
    src/input.c \
    src/encoding.c \
    src/line-output.c \
    src/format.c \
    src/line-start.c \
    src/tokens.c \
    src/directives.c \
    src/scanning.c \
    src/directive-names.c \
    src/lexer.c \
    src/numbers.c \
    src/constants.c \
    src/strings.c \
    src/includes.c \
    src/comments.c \
    src/macro-definition.c \
    src/condition.c \
    src/macro-expansion.c \
    src/macro-identifiers.c \
    src/macro-arguments.c \
    src/characters.c \
    src/messages.c \
    src/diagnostics.c \
    crt/legacy-ctype.c
ORIGINAL_BUILD = build
ORIGINAL_OUTPUT = $(ORIGINAL_BUILD)/rcpp.exe
ORIGINAL_MAP = $(ORIGINAL_BUILD)/rcpp.map
ORIGINAL_CPPFLAGS = /Isrc /Icrt /I$(RECON)/include
ORIGINAL_CFLAGS = /nologo /c /Od /G3
ORIGINAL_LDFLAGS = /nologo /subsystem:console /entry:rcpp_start
ORIGINAL_LDLIBS = $(ORIGINAL_TOOLCHAIN_ROOT)/lib/KERNEL32.LIB
ORIGINAL_INCLUDE_DIRS = src crt $(RECON)/include $(ORIGINAL_TOOLCHAIN_ROOT)/include
RECON_AUDIT_PROFILE = rcpp
RECON_IMAGE_BASE = 16777216
RECON_POLICY = policy-data.json
RECON_AUDIT_REGISTRY_METADATA = 1
RECON_METADATA_REBUILT_MAP = $(ORIGINAL_MAP)
RECON_AUDIT_FLAGS = -q --discovery cfg --check-ret --check-references --check-ownership --check-extents --check-accepted-extents
RECON_AUDIT_IDENTIFY_RUNTIME = 1
RECON_RUNTIME_OVERRIDE_OBJECTS = $(addprefix $(ORIGINAL_BUILD)/obj/crt/,runtime.c.obj arguments.c.obj startup.c.obj file-runtime.c.obj)
RECON_RUNTIME_OVERRIDE_MAP = $(ORIGINAL_BUILD)/rcpp.map
include $(RECON_TOOL_DIR)/recon-tool.mk
