RECON_TOOL_DIR = $(abspath ../../../tooling/recon-tool)
RECON = ../../../lib/recon
NATIVE_CC = gcc
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -DRECON_BORLAND_400_USER_HANDLER -DRECON_BORLAND_400_SIGNAL -DRECON_BORLAND_400_DOS_TIME -DRECON_BORLAND_400_STDOUT -I$(RECON)/include -I$(RECON)/native -Isrc
NATIVE_CFLAGS = -std=gnu89
BINARY_SOURCES = src/main.c src/messages.c src/stubs.c
BINARY_SUPPORT_SOURCES = $(addprefix $(RECON)/src/,compat.c crt.c decimal.c entry.c process.c trace.c win32.c)
RECON_ENTRY_SOURCES = $(BINARY_SOURCES)
RECON_ENTRY_SUPPORT_SOURCES = $(BINARY_SUPPORT_SOURCES)
ORIGINAL_TOOLCHAIN = bc4.0
ORIGINAL_CC = $(RECON_TOOL) original tool $(ORIGINAL_TOOLCHAIN) BCC32.EXE --
ORIGINAL_LINK = $(RECON_TOOL) original tool $(ORIGINAL_TOOLCHAIN) TLINK32.EXE --
ORIGINAL_SOURCES = $(BINARY_SOURCES)
ORIGINAL_BUILD = build
ORIGINAL_OUTPUT = build/bcc.exe
ORIGINAL_MAP = build/bcc.map
ORIGINAL_INCLUDE_DIRS = src $(RECON)/include $(ORIGINAL_TOOLCHAIN_ROOT)/BC4/INCLUDE
winpath = $(subst /,\,$(1))
ORIGINAL_COMPILE = $(ORIGINAL_CC) -c -v -Isrc '-I$(call winpath,$(RECON)/include)' '-o$(call winpath,$@)' '$(call winpath,$<)'
ORIGINAL_LINK_COMMAND = $(ORIGINAL_LINK) /Tpe /ap /m /s '@build\bcc.rsp'
RECON_ITERATION_ENABLED = 1
RECON_AUDIT_PROFILE = cl
RECON_IMAGE_BASE = 65536
RECON_AUDIT_FLAGS = --original ../../../toolchains/bc4.0/BC4/BIN/BCC32.EXE --rebuilt $(ORIGINAL_OUTPUT) --map $(ORIGINAL_MAP) --discovery cfg --map-format tlink32 --check-ret --check-references --check-ownership --check-extents
RECON_AUDIT_PREREQUISITES = $(ORIGINAL_OUTPUT) $(ORIGINAL_MAP)
RECON_AUDIT_REGISTRY_RUNTIME = 1
RECON_AUDIT_IDENTIFY_RUNTIME = 1
RECON_RUNTIME_INPUTS = $(ORIGINAL_TOOLCHAIN_ROOT)/BC4/LIB/CW32.LIB $(ORIGINAL_TOOLCHAIN_ROOT)/BC4/LIB/C0X32.OBJ
RECON_RUNTIME_FLAGS = --library $(ORIGINAL_TOOLCHAIN_ROOT)/BC4/LIB/CW32.LIB --object $(ORIGINAL_TOOLCHAIN_ROOT)/BC4/LIB/C0X32.OBJ
RECON_RUNTIME_MAP = $(ORIGINAL_MAP)
include $(RECON_TOOL_DIR)/recon-tool.mk

$(ORIGINAL_OUTPUT) $(ORIGINAL_MAP): build/bcc.rsp
build/bcc.rsp: Makefile $(ORIGINAL_OBJECTS)
	@mkdir -p $(@D)
	@printf '%s\n' '$(call winpath,$(ORIGINAL_TOOLCHAIN_ROOT)/BC4/LIB/C0X32.OBJ) $(foreach o,$(ORIGINAL_OBJECTS),$(call winpath,$(o))),build\bcc.exe,build\bcc.map,$(call winpath,$(ORIGINAL_TOOLCHAIN_ROOT)/BC4/LIB/CW32.LIB) $(call winpath,$(ORIGINAL_TOOLCHAIN_ROOT)/BC4/LIB/IMPORT32.LIB)' > $@
