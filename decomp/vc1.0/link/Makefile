RECON_TOOL_DIR = $(abspath ../../../tooling/recon-tool)
RECON = ../../../lib/recon
NATIVE_CC = clang
NATIVE_CXX = clang++
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -DRECON_NATIVE_TRACE -I$(RECON)/include -I$(RECON)/native
NATIVE_CFLAGS = -std=gnu89 -fms-extensions -fasm-blocks
BINARY_SOURCES = src/link.c \
    src/ide.c \
    src/comdat-diagnostics.c \
    src/demangle.c \
    src/omf.c \
    src/omf-debug.c \
    src/checksum.c \
    src/editor.c \
    src/editor-rebase.c \
    src/editor-bind.c \
    src/edit-stubs.c \
    src/dump.c \
    src/disassembler.c \
    src/disassembly-output.c \
    src/opcode-table.c \
    src/dump-stubs.c \
    src/resources.c \
    src/resource-legacy.c \
    src/resource-encoding.c \
    src/resource-serialization.c \
    src/codeview.c \
    src/debug-symbols.c \
    src/map-file.c \
    src/definition-file.c \
    src/definition-stubs.c \
    src/driver.c \
    src/files.c \
    src/linker.c \
    src/repro.c \
    src/image.c \
    src/symbols.c \
    src/external-output.c \
    src/section-layout.c \
    src/base-relocations.c \
    src/arguments.c \
    src/options.c \
    src/exports.c \
    src/export-directives.c \
    src/export-resolution.c \
    src/export-object.c \
    src/library.c \
    src/library-output.c \
    src/library-options.c \
    src/library-stubs.c \
    src/import-library.c \
    src/import-library-stubs.c \
    src/import-archive.c \
    src/import-members.c \
    src/imports.c \
    src/ordering.c \
    src/parse-switch.c \
    src/usage.c \
    src/dos-stub.c \
    src/link-pass.c \
    src/pass-one.c \
    src/pass-two.c \
    src/machine.c \
    src/i386-relocations.c \
    src/m68k-relocations.c \
    src/archive.c \
    src/coff-io.c \
    src/object.c \
    src/mac-object.c \
    src/mac-debug.c \
    src/comdat.c \
    src/comdat-selection.c \
    src/arena.c \
    src/modules.c \
    src/mac-sections.c \
    src/mac-fixups.c \
    src/text-buffer.c \
    src/diagnostics.c \
    src/argument-stubs.c \
    src/files-stubs.c \
    src/driver-stubs.c \
    src/machine-stubs.c \
    src/object-stubs.c \
    src/option-stubs.c
BINARY_SUPPORT_SOURCES = crt/runtime.c crt/anchors.c harness/native-runtime.c harness/native-host.c harness/fixture-artifacts.c harness/fixture-process.c $(wildcard $(RECON)/src/*.c)
NATIVE_LDFLAGS = -Wl,--wrap=write_import_members -Wl,--wrap=omf_convert_debug_symbols
ORIGINAL_TOOLCHAIN = 1.0-nt
ORIGINAL_SOURCES = $(BINARY_SOURCES) crt/anchors.c crt/runtime.c
ORIGINAL_BUILD = build
ORIGINAL_OUTPUT = build/link.exe
ORIGINAL_MAP = build/link.map
ORIGINAL_CPPFLAGS = /Isrc /I$(RECON)/include
ORIGINAL_CFLAGS = /nologo /c /Od /G3
ORIGINAL_LDFLAGS = /nologo /subsystem:console
ORIGINAL_INCLUDE_DIRS = src $(RECON)/include $(ORIGINAL_TOOLCHAIN_ROOT)/include
RECON_AUDIT_PROFILE = link
RECON_IMAGE_BASE = 65536
RECON_POLICY = policy-data.json
RECON_AUDIT_FLAGS = -q --include-evidenced-runtime --check-accepted-extents --check-ret --check-references --check-ownership --check-extents
RECON_AUDIT_REGISTRY_RUNTIME = 1
RECON_AUDIT_IDENTIFY_RUNTIME = 1
RECON_RUNTIME_OVERRIDE_OBJECTS = $(ORIGINAL_BUILD)/obj/crt/runtime.c.obj
RECON_RUNTIME_OVERRIDE_MAP = $(ORIGINAL_BUILD)/link.map
# Canonical executable uses the same App/16 historical CRT bodies. Fixture
# policies, providers, entry observers and process/artifact comparators stay out.
RECON_ENTRY_SOURCES = $(BINARY_SOURCES)
RECON_ENTRY_SUPPORT_SOURCES = crt/runtime.c crt/anchors.c harness/native-runtime.c \
    $(addprefix $(RECON)/src/,compat.c crt.c decimal.c process.c trace.c win32.c)
NATIVE_BINARY ?= build/LINK.EXE
NATIVE_BIN_FILES = messages=../../../toolchains/1.0-nt/bin/LINK.ERR
NATIVE_BIN_ENTRY_CPPFLAGS = -URECON_NATIVE_ENTRY
include $(RECON_TOOL_DIR)/recon-tool.mk
