RECON_TOOL_DIR = $(abspath ../../../tooling/recon-tool)
RECON = ../../../lib/recon
NATIVE_CC = gcc
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -DRECON_CRT_PROFILE_WATCOM_100A -I$(RECON)/include -I$(RECON)/native -Isrc -Iharness
NATIVE_CFLAGS = -std=gnu89
BINARY_SOURCES = $(sort $(wildcard src/*.c crt/*.c))
BINARY_SUPPORT_SOURCES = harness/startup.c $(addprefix $(RECON)/src/,compat.c crt.c decimal.c entry.c process.c trace.c win32.c)

ORIGINAL_TOOLCHAIN = watcom10.0a
ORIGINAL_SOURCES = $(BINARY_SOURCES)
ORIGINAL_BUILD = build
ORIGINAL_OUTPUT = build/WPP386.EXE
ORIGINAL_MAP = build/wpp386.map
ORIGINAL_CC = $(RECON_TOOL) original tool $(ORIGINAL_TOOLCHAIN) WCC386.EXE --
ORIGINAL_LINK = $(RECON_TOOL) original tool $(ORIGINAL_TOOLCHAIN) WLINK.EXE --
original-windows = $(subst /,\,$(1))
ORIGINAL_CPPFLAGS = -isrc -iharness '-i$(call original-windows,$(RECON)/include)' '-i$(call original-windows,$(ORIGINAL_TOOLCHAIN_ROOT)/h)' '-i$(call original-windows,$(ORIGINAL_TOOLCHAIN_ROOT)/h/NT)'
ORIGINAL_CFLAGS = -bt=nt -3r -od -d2 -s
ORIGINAL_COMPILE = $(ORIGINAL_CC) $(ORIGINAL_CPPFLAGS) $(ORIGINAL_CFLAGS) '-fo=$(call original-windows,$@)' '$(call original-windows,$<)'
ORIGINAL_LINK_COMMAND = $(ORIGINAL_LINK) format windows nt runtime console debug all option 'map=$(call original-windows,$(ORIGINAL_MAP))' name '$(call original-windows,$(ORIGINAL_OUTPUT))' $(foreach obj,$(ORIGINAL_OBJECTS),file '$(call original-windows,$(obj))') libpath '$(call original-windows,$(ORIGINAL_TOOLCHAIN_ROOT)/lib386)' libpath '$(call original-windows,$(ORIGINAL_TOOLCHAIN_ROOT)/lib386/NT)' library clib3r library nt
ORIGINAL_INCLUDE_DIRS = src harness $(RECON)/include $(ORIGINAL_TOOLCHAIN_ROOT)/h $(ORIGINAL_TOOLCHAIN_ROOT)/h/NT

RECON_AUDIT_REPORT = $(HOME)/.ai-tmp/msvc2/watcom10.0a/wpp386/graph-audit.json
RECON_AUDIT_PROFILE = cl
RECON_IMAGE_BASE = 0
RECON_CF_PROFILE = harness/cf-profile.json
RECON_POLICY = harness/graph-policy.json
RECON_AUDIT_REGISTRY_METADATA = 1
RECON_AUDIT_REGISTRY_RUNTIME = 1
RECON_AUDIT_FLAGS = --discovery cfg --rebuilt $(ORIGINAL_OUTPUT) --map $(ORIGINAL_MAP) --map-format watcom10 --rebuilt-runtime .build/rebuilt-runtime.json --check-ownership --check-extents --check-ret -q
RECON_AUDIT_PREREQUISITES = $(ORIGINAL_OUTPUT) .build/rebuilt-runtime.json
include $(RECON_TOOL_DIR)/recon-tool.mk

# Classified original CRT owners have no rebuilt App function counterparts.
.build/rebuilt-runtime.json:
	@mkdir -p $(@D)
	printf '[]\n' > $@

# Remap only this App translation unit; host/shim implementations stay clean.
.build/pic/src/iosupp.c.o: NATIVE_CPPFLAGS += -include harness/directory.h

.build/pic/src/srcfile.c.o: NATIVE_CPPFLAGS += -include harness/stream-boundary.h
