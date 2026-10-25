RECON_TOOL_DIR := $(abspath ../../../tooling/recon-tool)
RECON := ../../../lib/recon
BINARY_SOURCES := src/main.c
BINARY_SUPPORT_SOURCES := $(wildcard $(RECON)/src/*.c)
NATIVE_CPPFLAGS := -DRECON_NATIVE_TEST -I$(RECON)/include -I$(RECON)/native
NATIVE_CFLAGS := -std=gnu89

ORIGINAL_TOOLCHAIN := watcom10.0a
ORIGINAL_SOURCES := $(BINARY_SOURCES)
ORIGINAL_OUTPUT := build/WASM.EXE
ORIGINAL_MAP := build/wasm.map
ORIGINAL_CFLAGS := -bt=nt -3r -od -s -d2
ORIGINAL_CPPFLAGS := -i=src '-i=..\..\..\lib\recon\include'
ORIGINAL_INCLUDE_DIRS := src ../../../lib/recon/include ../../../toolchains/watcom10.0a/h
ORIGINAL_CC = $(RECON_TOOL) original tool $(ORIGINAL_TOOLCHAIN) WCC386 --
ORIGINAL_LINK = $(RECON_TOOL) original tool $(ORIGINAL_TOOLCHAIN) WLINK --
ORIGINAL_COMPILE = $(ORIGINAL_CC) $(ORIGINAL_CFLAGS) $(ORIGINAL_CPPFLAGS) '-fo=$(subst /,\,$@)' '$(subst /,\,$<)'
ORIGINAL_LINK_COMMAND = $(ORIGINAL_LINK) @build/wasm.lnk
include $(RECON_TOOL_DIR)/recon-tool.mk

$(ORIGINAL_LINK_OUTPUTS): build/wasm.lnk
build/wasm.lnk: $(ORIGINAL_OBJECTS) Makefile scripts/write-link.py
	@mkdir -p $(@D)
	python3 scripts/write-link.py $@ $(ORIGINAL_OUTPUT) $(ORIGINAL_MAP) $(ORIGINAL_OBJECTS)

.PHONY: original
original: win32
