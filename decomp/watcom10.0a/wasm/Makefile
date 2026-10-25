RECON_TOOL_DIR := $(abspath ../../../tooling/recon-tool)
RECON := ../../../lib/recon
BINARY_SOURCES := src/main.c src/stubs.c
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
include $(RECON_TOOL_DIR)/recon-tool.mk

.PHONY: original
original: win32
