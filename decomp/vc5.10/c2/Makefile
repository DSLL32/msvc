RECON_TOOL_DIR = $(abspath ../../../tooling/recon-tool)
RECON = ../../../lib/recon
NATIVE_CC = clang
NATIVE_CXX = clang++
NATIVE_CPPFLAGS = -DRECON_NATIVE_TRACE -DRECON_NATIVE_TEST -DRECON_DEFAULT_FMODE=0 -DRECON_CRT_PROFILE_MSVCRT_510 -Dmain=c2_main -I$(RECON)/include -I$(RECON)/native -Isrc -Icrt -Iharness
NATIVE_CFLAGS = -std=gnu89 -fms-extensions -fasm-blocks -Wno-ignored-attributes
BINARY_SOURCES = crt/math-error.c \
    src/stubs.c \
    crt/startup.c \
    crt/precision.c \
    src/main.c \
    crt/onexit.c \
    src/environment.c \
    src/options.c \
    src/option-table.c \
    src/option-match.c \
    src/integer-option.c \
    src/il-files.c \
    src/il-input.c \
    src/diagnostics.c \
    src/cleanup.c \
    src/symbols.c \
    src/nodes.c \
    src/constants.c \
    src/intrinsics.c \
    src/machine-init.c \
    src/sections.c \
    src/records.c \
    src/symbol-input.c \
    src/functions.c \
    src/function-records.c \
    src/register-sets.c \
    src/function-setup.c \
    src/function-reader.c \
    src/registers.c \
    src/code-records.c \
    src/assembly-output.c \
    src/output-callbacks.c \
    src/coff-output.c
BINARY_SUPPORT_SOURCES = $(addprefix $(RECON)/src/,compat.c crt.c decimal.c entry.c process.c trace.c win32.c)

ORIGINAL_TOOLCHAIN = 5.10
ORIGINAL_SOURCES = $(BINARY_SOURCES)
ORIGINAL_BUILD = build
ORIGINAL_OUTPUT = $(ORIGINAL_BUILD)/c2.exe
ORIGINAL_MAP = $(ORIGINAL_BUILD)/c2.map
ORIGINAL_CPPFLAGS = /Isrc /Icrt /I$(RECON)/include
ORIGINAL_CFLAGS = /nologo /c /Od /G3 /W3 /MD
ORIGINAL_LDFLAGS = /nologo /subsystem:console /entry:mainCRTStartup
ORIGINAL_LDLIBS = $(ORIGINAL_BUILD)/messages.res $(ORIGINAL_TOOLCHAIN_ROOT)/lib/KERNEL32.LIB $(ORIGINAL_TOOLCHAIN_ROOT)/lib/USER32.LIB
ORIGINAL_INCLUDE_DIRS = src crt $(RECON)/include $(ORIGINAL_TOOLCHAIN_ROOT)/include

RECON_AUDIT_PROFILE = cl
RECON_IMAGE_BASE = 4194304
RECON_POLICY = policy-data.json
RECON_AUDIT_REGISTRY_METADATA = 1
RECON_METADATA_REBUILT_MAP = $(ORIGINAL_BUILD)/c2.map
RECON_AUDIT_REGISTRY_RUNTIME = 1
RECON_AUDIT_FLAGS = -q --original $(abspath ../../../toolchains/5.10/bin/C2.EXE) --rebuilt $(abspath $(ORIGINAL_OUTPUT)) --map $(abspath $(ORIGINAL_BUILD)/c2.map) --discovery cfg --check-ret --check-imports --check-references --check-accepted-extents --check-ownership --check-extents
RECON_AUDIT_IDENTIFY_RUNTIME = 1
RECON_RUNTIME_INPUTS = $(ORIGINAL_TOOLCHAIN_ROOT)/lib/LIBC.LIB
RECON_RUNTIME_FLAGS = --library $(ORIGINAL_TOOLCHAIN_ROOT)/lib/LIBC.LIB
RECON_RUNTIME_OVERRIDE_OBJECTS = $(addprefix $(ORIGINAL_BUILD)/obj/crt/,startup.c.obj onexit.c.obj precision.c.obj)
RECON_RUNTIME_OVERRIDE_MAP = $(ORIGINAL_BUILD)/c2.map
include $(RECON_TOOL_DIR)/recon-tool.mk

# Preserve encountered compiler intrinsic choices and the genuine CRT thunk.
$(addprefix $(ORIGINAL_BUILD)/obj/src/,option-match.c.obj diagnostics.c.obj il-files.c.obj symbols.c.obj nodes.c.obj symbol-input.c.obj coff-output.c.obj function-records.c.obj function-setup.c.obj): ORIGINAL_CFLAGS += /Oi
$(ORIGINAL_BUILD)/obj/crt/precision.c.obj: ORIGINAL_CFLAGS = /nologo /c /Od /G3 /W3 /ML /Zl
$(ORIGINAL_BUILD)/messages.res: src/messages.rc $(RECON_TOOL)
	@mkdir -p $(@D)
	$(RECON_TOOL) original tool 5.10 RC.EXE -- /r /fo $@ $<
