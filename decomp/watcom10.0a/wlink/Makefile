RECON_TOOL_DIR := $(abspath ../../../tooling/recon-tool)
RECON := ../../../lib/recon
NATIVE_CC := gcc
NATIVE_CPPFLAGS := -DRECON_NATIVE_TEST -DRECON_CRT_PROFILE_WATCOM_100A -DRECON_NATIVE_STARTUP_WATCOM_100A_FP -I$(RECON)/include -I$(RECON)/native
NATIVE_CFLAGS := -std=gnu89 -fms-extensions
BINARY_SOURCES := src/main.c
BINARY_SUPPORT_SOURCES := $(addprefix $(RECON)/src/,compat.c crt.c decimal.c entry.c process.c trace.c win32.c)
ORIGINAL_TOOLCHAIN := watcom10.0a
ORIGINAL_SOURCES := $(BINARY_SOURCES)
ORIGINAL_BUILD := build
ORIGINAL_OUTPUT := $(ORIGINAL_BUILD)/WLINK.EXE
ORIGINAL_MAP := $(ORIGINAL_BUILD)/wlink.map
ORIGINAL_CPPFLAGS := -i=src '-i=..\..\..\lib\recon\include'
ORIGINAL_CFLAGS := -bt=nt -3r -od -s -d2
ORIGINAL_CC = $(RECON_TOOL) original tool $(ORIGINAL_TOOLCHAIN) WCC386.EXE --
ORIGINAL_LINK = $(RECON_TOOL) original tool $(ORIGINAL_TOOLCHAIN) WLINK.EXE --
ORIGINAL_COMPILE = $(ORIGINAL_CC) $(ORIGINAL_CPPFLAGS) $(ORIGINAL_CFLAGS) '-fo=$(subst /,\,$@)' '$(subst /,\,$<)'
define ORIGINAL_LINK_COMMAND
python3 scripts/write-link.py --response $(ORIGINAL_BUILD)/wlink.lnk --output $(ORIGINAL_OUTPUT) --map $(ORIGINAL_MAP) $(ORIGINAL_OBJECTS)
$(ORIGINAL_LINK) @$(ORIGINAL_BUILD)/wlink.lnk
endef
ORIGINAL_INCLUDE_DIRS := src $(RECON)/include ../../../toolchains/watcom10.0a/h
ORIGINAL_EXTRA_DEPS := scripts/write-link.py
include $(RECON_TOOL_DIR)/recon-tool.mk
