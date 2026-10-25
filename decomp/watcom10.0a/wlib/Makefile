RECON_TOOL_DIR = $(abspath ../../../tooling/recon-tool)
RECON = ../../../lib/recon
NATIVE_CC = gcc
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -DRECON_CRT_PROFILE_WATCOM_100A -DRECON_NATIVE_STARTUP_WATCOM_100A_FP -I$(RECON)/include -I$(RECON)/native -Isrc
NATIVE_CFLAGS = -std=gnu89
BINARY_SOURCES = $(wildcard src/*.c)
BINARY_SUPPORT_SOURCES = harness/heap-contract.c $(addprefix $(RECON)/src/,compat.c crt.c decimal.c entry.c process.c trace.c win32.c)
ORIGINAL_TOOLCHAIN = watcom10.0a
ORIGINAL_CC = $(RECON_TOOL) original tool $(ORIGINAL_TOOLCHAIN) WCC386.EXE --
ORIGINAL_LINK = $(RECON_TOOL) original tool $(ORIGINAL_TOOLCHAIN) WLINK.EXE --
ORIGINAL_SOURCES = $(BINARY_SOURCES)
ORIGINAL_BUILD = build
ORIGINAL_OUTPUT = build/WLIB.EXE
ORIGINAL_MAP = build/wlib.map
ORIGINAL_INCLUDE_DIRS = src $(RECON)/include $(ORIGINAL_TOOLCHAIN_ROOT)/h
winpath = $(subst /,\,$(1))
ORIGINAL_COMPILE = $(ORIGINAL_CC) -bt=nt -3r -od -d2 -s '-i=$(call winpath,$(RECON)/include)' '-i=$(call winpath,$(ORIGINAL_TOOLCHAIN_ROOT)/h)' '-fo=$(call winpath,$@)' '$(call winpath,$<)'
ORIGINAL_LINK_COMMAND = $(ORIGINAL_LINK) '@build\wlib.lnk'
RECON_POLICY = policy-data.json
RECON_AUDIT_PREREQUISITES = harness/rebuilt-runtime.json harness/ret-runtime-original.json harness/readseek-runtime.json harness/call-identity.json $(ORIGINAL_OUTPUT) $(ORIGINAL_MAP)
RECON_AUDIT_PROFILE = cl
RECON_IMAGE_BASE = 4194304
# App-body audit. The finite original lseek record supplies the RET convention
# inherited by res_seek's tail branch; no support-body equivalence is asserted.
RECON_AUDIT_FLAGS = --discovery cfg --map-format watcom10 --original ../../../toolchains/watcom10.0a/.patched/WLIB.EXE --rebuilt $(ORIGINAL_OUTPUT) --map $(ORIGINAL_MAP) --check-ret --check-ownership --check-extents --runtime harness/ret-runtime-original.json --rebuilt-runtime harness/rebuilt-runtime.json
include $(RECON_TOOL_DIR)/recon-tool.mk

# Period-linker input; original commands use only the common tool launcher.
$(ORIGINAL_OUTPUT) $(ORIGINAL_MAP): build/wlib.lnk
build/wlib.lnk: Makefile $(ORIGINAL_OBJECTS)
	@mkdir -p $(@D)
	@printf '%s\n' 'format windows nt' 'runtime console' 'debug all' 'option map=build\wlib.map' 'name build\WLIB.EXE' $(foreach o,$(ORIGINAL_OBJECTS),'file $(call winpath,$(o))') 'libpath $(call winpath,$(ORIGINAL_TOOLCHAIN_ROOT)/lib386)' 'libpath $(call winpath,$(ORIGINAL_TOOLCHAIN_ROOT)/lib386/NT)' 'library clib3r' 'library nt' > $@
