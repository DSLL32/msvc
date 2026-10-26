RECON_TOOL_DIR := $(abspath ../../../tooling/recon-tool)
RECON := ../../../lib/recon
NATIVE_CC := gcc
NATIVE_CPPFLAGS := -DRECON_NATIVE_TEST -DRECON_CRT_PROFILE_WATCOM_100A -DRECON_NATIVE_STARTUP_WATCOM_100A_FP -I$(RECON)/include -I$(RECON)/native
NATIVE_CFLAGS := -std=gnu89 -fms-extensions
BINARY_SOURCES := $(wildcard src/*.c)
BINARY_SUPPORT_SOURCES := $(addprefix $(RECON)/src/,compat.c crt.c decimal.c entry.c process.c trace.c win32.c)
ORIGINAL_TOOLCHAIN := watcom10.0a
ORIGINAL_SOURCES := $(BINARY_SOURCES)
ORIGINAL_BUILD := build
ORIGINAL_OUTPUT := $(ORIGINAL_BUILD)/WLINK.EXE
ORIGINAL_MAP := $(ORIGINAL_BUILD)/wlink.map
ORIGINAL_CPPFLAGS := -i=src '-i=..\..\..\lib\recon\include' '-i=..\..\..\toolchains\watcom10.0a\h'
ORIGINAL_CFLAGS := -bt=nt -3r -od -s -d2
ORIGINAL_CC = $(RECON_TOOL) original tool $(ORIGINAL_TOOLCHAIN) WCC386.EXE --
ORIGINAL_LINK = $(RECON_TOOL) original tool $(ORIGINAL_TOOLCHAIN) WLINK.EXE --
ORIGINAL_COMPILE = $(ORIGINAL_CC) $(ORIGINAL_CPPFLAGS) $(ORIGINAL_CFLAGS) '-fo=$(subst /,\,$@)' '$(subst /,\,$<)'
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
ORIGINAL_LINK_COMMAND = $(file >$(ORIGINAL_BUILD)/wlink.lnk,$(WATCOM_LINK_RESPONSE))$(ORIGINAL_LINK) @$(ORIGINAL_BUILD)/wlink.lnk
ORIGINAL_INCLUDE_DIRS := src $(RECON)/include ../../../toolchains/watcom10.0a/h
RECON_POLICY := policy-data.json
RECON_AUDIT_REGISTRY_RET_CHECKS := 1
RECON_METADATA_REBUILT_MAP := $(ORIGINAL_MAP)
RECON_AUDIT_PROFILE := cl
RECON_AUDIT_REPORT := $(HOME)/.ai-tmp/msvc2/watcom10.0a/wlink/audit/graph-audit.json
RECON_IMAGE_BASE := 4194304
RECON_AUDIT_REGISTRY_RUNTIME := 1
RECON_AUDIT_PREREQUISITES := .build/runtime-rebuilt.json $(ORIGINAL_OUTPUT) $(ORIGINAL_MAP)
RECON_AUDIT_FLAGS := --discovery cfg --map-format watcom10 --original ../../../toolchains/watcom10.0a/.patched/WLINK.EXE --rebuilt $(ORIGINAL_OUTPUT) --map $(ORIGINAL_MAP) --rebuilt-runtime .build/runtime-rebuilt.json --check-references --check-ownership --check-extents
include $(RECON_TOOL_DIR)/recon-tool.mk

# Genuine main ends in exit; returning application functions retain strict RET audits.
.PHONY: audit-main-endpoint
audit: audit-main-endpoint
audit-main-endpoint: recon-graph-audit
	python3 scripts/check-main-endpoint.py $(RECON_AUDIT_REPORT)

# Rebind the finite module end and shipped longjmp catalog after a build.
$(RECON_POLICY) .build/runtime-rebuilt.json &: $(ORIGINAL_OUTPUT) $(ORIGINAL_MAP) scripts/update-lnkinit-extent.py
	python3 scripts/update-lnkinit-extent.py
