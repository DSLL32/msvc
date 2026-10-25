RECON_TOOL_DIR = $(abspath ../../../tooling/recon-tool)
RECON = ../../../lib/recon
NATIVE_CC = gcc
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -I$(RECON)/include -I$(RECON)/native -Isrc -Iharness
NATIVE_CFLAGS = -std=gnu89
BINARY_SOURCES = $(sort $(wildcard src/*.c))
BINARY_SUPPORT_SOURCES = harness/startup.c $(wildcard $(RECON)/src/*.c)

ORIGINAL_TOOLCHAIN = watcom10.0a
ORIGINAL_SOURCES = $(BINARY_SOURCES) harness/original-host.c
ORIGINAL_OUTPUT = build/WCC386.EXE
ORIGINAL_MAP = build/wcc386.map
ORIGINAL_CPPFLAGS = '-i=..\..\..\toolchains\watcom10.0a\h' '-i=..\..\..\toolchains\watcom10.0a\h\nt' '-i=src' '-i=..\..\..\lib\recon\include'
ORIGINAL_CFLAGS = -bt=nt -3r -od -d2 -s
ORIGINAL_CC = $(RECON_TOOL) original tool watcom10.0a WCC386.EXE --
ORIGINAL_LINK = $(RECON_TOOL) original tool watcom10.0a WLINK.EXE --
ORIGINAL_COMPILE = $(ORIGINAL_CC) $(ORIGINAL_CPPFLAGS) $(ORIGINAL_CFLAGS) '-fo=$(subst /,\,$@)' '$(subst /,\,$<)'
ORIGINAL_LINK_COMMAND = $(ORIGINAL_LINK) '@build\wcc386.lnk'
ORIGINAL_LDLIBS = build/wcc386.lnk
ORIGINAL_INCLUDE_DIRS = src $(RECON)/include
RECON_AUDIT_PROFILE = cl
RECON_IMAGE_BASE = 0
RECON_CF_PROFILE = cf-profile.json
RECON_POLICY = policy-data.json
RECON_AUDIT_REGISTRY_METADATA = 1
RECON_AUDIT_REGISTRY_RUNTIME = 1
RECON_AUDIT_REPORT = build/memory-graph.json
RECON_AUDIT_PREREQUISITES = $(ORIGINAL_OUTPUT) $(ORIGINAL_MAP) build/rebuilt-runtime.json
RECON_AUDIT_FLAGS = -q --discovery cfg --check-ownership --check-extents --check-ret --export-instructions --rebuilt $(ORIGINAL_OUTPUT) --map $(ORIGINAL_MAP) --map-format watcom10 --rebuilt-runtime build/rebuilt-runtime.json
include $(RECON_TOOL_DIR)/recon-tool.mk

build/wcc386.lnk: Makefile $(ORIGINAL_SOURCES)
	@mkdir -p $(@D)
	printf '%s\n' 'format windows nt' 'runtime console' 'debug all' 'option map=build\wcc386.map' 'name build\WCC386.EXE' $(foreach object,$(ORIGINAL_OBJECTS),'file $(subst /,\,$(object))') 'libpath $(subst /,\,$(ORIGINAL_TOOLCHAIN_ROOT)/lib386)' 'libpath $(subst /,\,$(ORIGINAL_TOOLCHAIN_ROOT)/lib386/nt)' 'library clib3r' 'library nt' > $@

.PHONY: original paired
original: win32
paired: cases $(RECON_TOOL)
	$(RECON_TOOL) run cases/0000-help --capture cases/0000-help/capture.bin --original-report cases/0000-help/original-report.json --report build/native-frontier.json

build/rebuilt-runtime.json:
	@mkdir -p $(@D)
	printf '[]\n' > $@
