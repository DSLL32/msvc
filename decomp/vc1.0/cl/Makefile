RECON_TOOL_DIR = $(abspath ../../../tooling/recon-tool)
RECON = ../../../lib/recon
NATIVE_CC = clang
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -I$(RECON)/include -I$(RECON)/native -Isrc
NATIVE_CFLAGS = -std=gnu89 -fms-extensions
BINARY_SOURCES = src/cl.c src/stubs.c src/data.c src/driver.c src/remaining.c
BINARY_SUPPORT_SOURCES = crt/runtime.c harness/child-contract.c $(addprefix $(RECON)/src/,compat.c crt.c decimal.c entry.c process.c trace.c win32.c)
# Canonical execution uses real sibling processes; the captured child policy
# belongs only to case libraries.
RECON_ENTRY_SOURCES = $(BINARY_SOURCES)
RECON_ENTRY_SUPPORT_SOURCES = crt/runtime.c $(addprefix $(RECON)/src/,compat.c crt.c decimal.c entry.c process.c trace.c win32.c)
NATIVE_BIN_FILES = cl_errors=../../../toolchains/1.0-nt/bin/CL.ERR \
    cl_help=../../../toolchains/1.0-nt/bin/CL32.MSG \
    c1_errors=../../../toolchains/1.0-nt/bin/C1.ERR \
    c2_errors=../../../toolchains/1.0-nt/bin/C23.ERR
ORIGINAL_TOOLCHAIN = 1.0-nt
ORIGINAL_SOURCES = $(BINARY_SOURCES) crt/runtime.c
ORIGINAL_BUILD = build
ORIGINAL_OUTPUT = $(ORIGINAL_BUILD)/cl.exe
ORIGINAL_MAP = $(ORIGINAL_BUILD)/cl.map
ORIGINAL_CPPFLAGS = /Isrc /I$(RECON)/include
ORIGINAL_CFLAGS = /nologo /c /Od /G3
ORIGINAL_LDFLAGS = /nologo /subsystem:console
ORIGINAL_LDLIBS = $(ORIGINAL_TOOLCHAIN_ROOT)/lib/SETARGV.OBJ
ORIGINAL_INCLUDE_DIRS = src $(RECON)/include $(ORIGINAL_TOOLCHAIN_ROOT)/include
RECON_AUDIT_PROFILE = cl
RECON_IMAGE_BASE = 65536
RECON_AUDIT_FLAGS = --discovery cfg --check-ret --check-ownership --check-extents
RECON_AUDIT_REGISTRY_RUNTIME = 1
RECON_AUDIT_IDENTIFY_RUNTIME = 1
RECON_RUNTIME_OVERRIDE_OBJECTS = $(ORIGINAL_BUILD)/obj/crt/runtime.c.obj
RECON_RUNTIME_OVERRIDE_MAP = $(ORIGINAL_MAP)
include $(RECON_TOOL_DIR)/recon-tool.mk
.PHONY: reference-build
reference-build: recon-original
