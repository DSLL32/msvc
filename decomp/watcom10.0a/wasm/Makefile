RECON_TOOL_DIR := $(abspath ../../../tooling/recon-tool)
RECON := ../../../lib/recon
RECON_CF_PROFILE := harness/cf-profile.json
BINARY_SOURCES := src/main.c src/fatal.c src/write.c src/objrec.c src/objio.c src/carve.c src/fixup.c src/memutil.c src/asmalloc.c src/genmsomf.c src/util.c src/wasmmsg.c src/loadstr.c src/ropenfro.c src/wrinitdi.c src/wrreaddi.c src/wriswrf.c src/resread.c src/wrrhr.c src/wresid.c src/reslist.c src/asmins.c src/asmparse.c src/asmmatch.c src/breakout.c src/asmstruc.c src/mnemonics.c src/insthash.c src/expand.c src/asmline.c src/condasm.c src/macro.c src/mangle.c src/asmscan.c src/asmsym.c src/asmlabel.c src/asmjump.c src/direct.c src/asmeval.c src/asmdata.c src/asmfixup.c src/posndir.c src/wrfindrs.c src/stubs.c
BINARY_SUPPORT_SOURCES := $(wildcard $(RECON)/src/*.c)
NATIVE_CPPFLAGS := -DRECON_CRT_PROFILE_WATCOM_100A -DRECON_SPLITPATH_WATCOM_DOS -DRECON_NATIVE_TEST -I$(RECON)/include -I$(RECON)/native
NATIVE_CFLAGS := -std=gnu89

ORIGINAL_TOOLCHAIN := watcom10.0a
ORIGINAL_SOURCES := $(BINARY_SOURCES)
ORIGINAL_OUTPUT := build/WASM.EXE
ORIGINAL_MAP := build/wasm.map
# Actual original double values use EAX/EDX and soft-float library calls.
ORIGINAL_CFLAGS := -bt=nt -3r -fpc -od -s -d2
ORIGINAL_CPPFLAGS := -i=src '-i=..\..\..\lib\recon\include' '-i=..\..\..\toolchains\watcom10.0a\h'
ORIGINAL_INCLUDE_DIRS := src ../../../lib/recon/include ../../../toolchains/watcom10.0a/h
ORIGINAL_CC = $(RECON_TOOL) original tool $(ORIGINAL_TOOLCHAIN) WCC386 --
ORIGINAL_LINK = $(RECON_TOOL) original tool $(ORIGINAL_TOOLCHAIN) WLINK --
ORIGINAL_COMPILE = $(ORIGINAL_CC) $(ORIGINAL_CFLAGS) $(ORIGINAL_CPPFLAGS) '-fo=$(subst /,\,$@)' '$(subst /,\,$<)'
# The old linker consumes directive lines rather than MSVC switches.
define WATCOM_NEWLINE


endef
define WATCOM_LINK_RESPONSE
format windows nt
runtime console
debug all
option map=$(subst /,\,$(ORIGINAL_MAP))
name $(subst /,\,$(ORIGINAL_OUTPUT))$(foreach object,$(ORIGINAL_OBJECTS),$(WATCOM_NEWLINE)file $(subst /,\,$(object)))
libpath R:\toolchains\watcom10.0a\lib386
libpath R:\toolchains\watcom10.0a\lib386\nt
library clib3r
library nt
endef
ORIGINAL_LINK_COMMAND = $(file >$(ORIGINAL_BUILD)/wasm.lnk,$(WATCOM_LINK_RESPONSE))$(ORIGINAL_LINK) @$(ORIGINAL_BUILD)/wasm.lnk
# Semantic audit policy only; generated projections/reports stay in scratch.
WASM_AUDIT_DIR := $(HOME)/.ai-tmp/msvc2/watcom10.0a/wasm/audit
RECON_AUDIT_PROFILE := cl
RECON_IMAGE_BASE := 0
RECON_POLICY := harness/audit-policy.json
RECON_AUDIT_REGISTRY_METADATA := 1
RECON_METADATA_REBUILT_MAP := $(ORIGINAL_MAP)
RECON_AUDIT_REGISTRY_RET_CHECKS := 1
RECON_AUDIT_REGISTRY_RUNTIME := 1
RECON_REGISTRY := $(WASM_AUDIT_DIR)/functions.json
RECON_AUDIT_METADATA := $(WASM_AUDIT_DIR)/metadata.json
RECON_AUDIT_RET_CHECKS := $(WASM_AUDIT_DIR)/ret-checks.json
RECON_AUDIT_ORIGINAL_RUNTIME := $(WASM_AUDIT_DIR)/runtime-original.json
RECON_AUDIT_REPORT := $(WASM_AUDIT_DIR)/graph-audit.json
RECON_AUDIT_PREREQUISITES := $(ORIGINAL_OUTPUT) $(ORIGINAL_MAP) $(WASM_AUDIT_DIR)/rebuilt-runtime.json
RECON_AUDIT_FLAGS := -q --discovery cfg --rebuilt $(ORIGINAL_OUTPUT) --map $(ORIGINAL_MAP) --map-format watcom10 --rebuilt-runtime $(WASM_AUDIT_DIR)/rebuilt-runtime.json --check-ownership --check-extents --check-accepted-extents --check-references --check-instruction-overlap
include $(RECON_TOOL_DIR)/recon-tool.mk

# Original CRT owners have no rebuilt application-body counterparts.
$(WASM_AUDIT_DIR)/rebuilt-runtime.json:
	@mkdir -p $(@D)
	@printf '[]\n' > $@

#258 ordinary returning contracts stay strict;71 exact cold-error/tail/exit
# owners use the finite residual without inheriting a fictitious cold RET.
.PHONY: audit-error-tails
audit: audit-error-tails
audit-error-tails: $(ORIGINAL_OUTPUT) $(ORIGINAL_MAP) scripts/check-error-tails.py $(RECON_CF_PROFILE)
	python3 scripts/check-error-tails.py $(WASM_AUDIT_DIR)/error-tails.json

.PHONY: original
original: win32
