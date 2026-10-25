RECON_TOOL_DIR = $(abspath ../../../tooling/recon-tool)
RECON = ../../../lib/recon
NATIVE_CC = gcc
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -I$(RECON)/include -I$(RECON)/native -Isrc -Iharness
NATIVE_CFLAGS = -std=gnu89
BINARY_SOURCES = src/stubs.c
BINARY_SUPPORT_SOURCES = harness/startup.c $(addprefix $(RECON)/src/,compat.c crt.c decimal.c entry.c process.c trace.c win32.c)

ORIGINAL_TOOLCHAIN = watcom10.0a
ORIGINAL_SOURCES = $(BINARY_SOURCES)
ORIGINAL_BUILD = build
ORIGINAL_OUTPUT = build/WPP386.EXE
ORIGINAL_MAP = build/wpp386.map
ORIGINAL_CC = $(RECON_TOOL) original tool $(ORIGINAL_TOOLCHAIN) WCC386.EXE --
ORIGINAL_LINK = $(RECON_TOOL) original tool $(ORIGINAL_TOOLCHAIN) WLINK.EXE --
original-windows = $(subst /,\,$(1))
ORIGINAL_CPPFLAGS = -isrc '-i$(call original-windows,$(RECON)/include)'
ORIGINAL_CFLAGS = -bt=nt -3r -od -d2
ORIGINAL_COMPILE = $(ORIGINAL_CC) $(ORIGINAL_CPPFLAGS) $(ORIGINAL_CFLAGS) '-fo=$(call original-windows,$@)' '$(call original-windows,$<)'
ORIGINAL_LINK_COMMAND = $(ORIGINAL_LINK) format windows nt runtime console debug all option 'map=$(call original-windows,$(ORIGINAL_MAP))' name '$(call original-windows,$(ORIGINAL_OUTPUT))' $(foreach obj,$(ORIGINAL_OBJECTS),file '$(call original-windows,$(obj))') libpath '$(call original-windows,$(ORIGINAL_TOOLCHAIN_ROOT)/lib386)' libpath '$(call original-windows,$(ORIGINAL_TOOLCHAIN_ROOT)/lib386/NT)' library clib3r library nt
ORIGINAL_INCLUDE_DIRS = src $(RECON)/include $(ORIGINAL_TOOLCHAIN_ROOT)/h $(ORIGINAL_TOOLCHAIN_ROOT)/h/NT

RECON_AUDIT_PROFILE = cl
RECON_IMAGE_BASE = 0
RECON_CF_PROFILE = harness/cf-identity.json
RECON_POLICY = harness/graph-policy.json
RECON_AUDIT_REGISTRY_METADATA = 1
RECON_AUDIT_REGISTRY_RUNTIME = 1
RECON_AUDIT_FLAGS = --discovery cfg --rebuilt $(ORIGINAL_OUTPUT) --map $(ORIGINAL_MAP) --map-format watcom10 --rebuilt-runtime .build/rebuilt-runtime.json --check-ownership --check-extents --export-original-support-instructions _cstart_ --export-original-support-instructions __InitRtns --export-original-support-instructions _init_stk --export-original-support-instructions __InitFiles_ --export-original-support-instructions malloc_segment_wrapper -q
RECON_AUDIT_PREREQUISITES = $(ORIGINAL_OUTPUT) .build/rebuilt-runtime.json
include $(RECON_TOOL_DIR)/recon-tool.mk

# The five original-only support owners have no rebuilt function counterparts.
# Their return contracts are separately witness-bound; no paired ABI is claimed.
.build/rebuilt-runtime.json:
	@mkdir -p $(@D)
	printf '[]\n' > $@
