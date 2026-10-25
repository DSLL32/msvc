PYTHON ?= python3
RECON_TOOL_DIR = $(abspath ../../../tooling/recon-tool)
RECON = ../../../lib/recon
NATIVE_CC = clang
NATIVE_CXX = clang++
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -DRECON_NATIVE_TRACE -I$(RECON)/include -I$(RECON)/native -Isrc -Icrt -Iharness
NATIVE_CFLAGS = -std=gnu89 -fms-extensions -fasm-blocks -fshort-wchar
BINARY_SOURCES = src/heap.c \
    src/driver.c \
    crt/character-case.c \
    src/characters.c \
    src/prepare.c \
    src/files.c \
    src/lexer.c \
    src/resources.c \
    src/resource-types.c \
    crt/wide-copy.c \
    src/resource-data.c \
    src/identifiers.c \
    src/tokens.c \
    src/numbers.c \
    src/language.c \
    src/keywords.c \
    src/conversion.c \
    src/resource-flags.c \
    src/diagnostics.c \
    src/menu.c \
    src/strings.c \
    src/parser-helpers.c \
    src/resource-writer.c \
    src/dialog.c \
    src/style.c \
    src/dialog-properties.c \
    src/wide-numbers.c \
    src/dialog-controls.c \
    src/control-fields.c \
    src/control-kinds.c \
    src/control-generic.c \
    src/string-tables.c \
    src/accelerators.c \
    src/images.c \
    src/resource-files.c \
    src/bitmaps.c \
    src/raw-data.c \
    src/wide-strings.c \
    src/dialog-include.c \
    src/version.c \
    src/finish.c \
    src/fonts.c \
    src/legacy-images.c \
    src/kanji-strings.c \
    src/unused-helpers.c
BINARY_SUPPORT_SOURCES = $(wildcard $(RECON)/src/*.c)
ORIGINAL_TOOLCHAIN = 1.0-nt
ORIGINAL_SOURCES = crt/startup.c \
    src/heap.c \
    crt/heap-crt.c \
    crt/arguments.c \
    src/driver.c \
    crt/character-case.c \
    src/characters.c \
    crt/paths.c \
    crt/open.c \
    crt/lowio.c \
    src/prepare.c \
    crt/format.c \
    src/files.c \
    crt/stdio.c \
    crt/process.c \
    src/lexer.c \
    src/resources.c \
    src/resource-types.c \
    crt/wide-copy.c \
    src/resource-data.c \
    src/identifiers.c \
    src/tokens.c \
    src/numbers.c \
    src/language.c \
    src/keywords.c \
    crt/atol.c \
    src/conversion.c \
    src/resource-flags.c \
    src/diagnostics.c \
    src/menu.c \
    src/strings.c \
    src/parser-helpers.c \
    src/resource-writer.c \
    src/dialog.c \
    src/style.c \
    src/dialog-properties.c \
    src/wide-numbers.c \
    src/dialog-controls.c \
    src/control-fields.c \
    src/control-kinds.c \
    src/control-generic.c \
    src/string-tables.c \
    src/accelerators.c \
    src/images.c \
    src/resource-files.c \
    src/bitmaps.c \
    src/raw-data.c \
    src/wide-strings.c \
    src/dialog-include.c \
    src/version.c \
    src/finish.c \
    crt/crt-state.c \
    crt/termination.c \
    crt/flush-all.c \
    crt/temporary-buffer.c \
    src/fonts.c \
    src/legacy-images.c \
    crt/scan-input.c \
    src/kanji-strings.c \
    crt/spawn-search.c \
    src/unused-helpers.c
ORIGINAL_BUILD = build
ORIGINAL_OUTPUT = $(ORIGINAL_BUILD)/rc.exe
ORIGINAL_MAP = $(ORIGINAL_BUILD)/rc.map
ORIGINAL_CPPFLAGS = /I$(RECON)/include /Isrc /Icrt
ORIGINAL_CFLAGS = /nologo /c /Od /G3
ORIGINAL_LDFLAGS = /nologo /entry:mainCRTStartup /subsystem:console
ORIGINAL_LDLIBS = build/ntdll.lib $(ORIGINAL_TOOLCHAIN_ROOT)/lib/LIBC.LIB
ORIGINAL_INCLUDE_DIRS = src crt $(RECON)/include $(ORIGINAL_TOOLCHAIN_ROOT)/include
RECON_AUDIT_PROFILE = rc
RECON_IMAGE_BASE = 16777216
RECON_POLICY = policy-data.json
RECON_AUDIT_FLAGS = -q --discovery cfg --check-ret --check-references --check-callbacks --check-ownership --check-extents --check-accepted-extents --check-registry-support-extents --check-runtime-data
RECON_AUDIT_REGISTRY_METADATA = 1
RECON_AUDIT_PREREQUISITES = runtime.json
RECON_AUDIT_FLAGS += --runtime runtime.json
RECON_AUDIT_IDENTIFY_RUNTIME = 1
RECON_RUNTIME_INPUTS = $(ORIGINAL_TOOLCHAIN_ROOT)/lib/LIBC.LIB $(ORIGINAL_TOOLCHAIN_ROOT)/lib/LIBCMT.LIB $(ORIGINAL_TOOLCHAIN_ROOT)/lib/SETARGV.OBJ
RECON_RUNTIME_FLAGS = --library LIBC.LIB --library LIBCMT.LIB --object SETARGV.OBJ
RECON_RUNTIME_OVERRIDE_MAP = $(ORIGINAL_MAP)
RECON_RUNTIME_OVERRIDE_OBJECTS = $(addprefix $(ORIGINAL_BUILD)/obj/crt/,flush-all.c.obj process.c.obj heap-crt.c.obj)
include $(RECON_TOOL_DIR)/recon-tool.mk

# This toolchain lacks the two NT conversion imports. Only their hint/name
# records need fixing; source symbols remain decorated for original CL/LINK.
build/ntdll.lib: src/ntdll.def scripts/import-library.py | $(RECON_TOOL)
	@mkdir -p $(@D)
	$(RECON_TOOL) original tool 1.0-nt LIB.EXE -- /nologo /def:src/ntdll.def /out:$@ /machine:IX86
	$(PYTHON) scripts/import-library.py $@
