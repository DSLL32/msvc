RECON_TOOL_DIR = $(abspath ../../../tooling/recon-tool)
RECON = ../../../lib/recon
NATIVE_CC = gcc
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -I$(RECON)/include -I$(RECON)/native -Isrc -Iharness
NATIVE_CFLAGS = -std=gnu89
BINARY_SOURCES = $(sort $(wildcard src/*.c))
BINARY_SUPPORT_SOURCES = harness/startup.c $(RECON)/src/trace.c

ORIGINAL_TOOLCHAIN = watcom10.0a
ORIGINAL_SOURCES = $(BINARY_SOURCES) harness/original-host.c
ORIGINAL_OUTPUT = build/WCC386.EXE
ORIGINAL_MAP = build/wcc386.map
ORIGINAL_CPPFLAGS = '-i=src' '-i=..\..\..\lib\recon\include'
ORIGINAL_CFLAGS = -bt=nt -3r -od -d2 -s
ORIGINAL_CC = $(RECON_TOOL) original tool watcom10.0a WCC386.EXE --
ORIGINAL_LINK = $(RECON_TOOL) original tool watcom10.0a WLINK.EXE --
ORIGINAL_COMPILE = $(ORIGINAL_CC) $(ORIGINAL_CPPFLAGS) $(ORIGINAL_CFLAGS) '-fo=$(subst /,\,$@)' '$(subst /,\,$<)'
ORIGINAL_LINK_COMMAND = $(ORIGINAL_LINK) '@build\wcc386.lnk'
ORIGINAL_LDLIBS = build/wcc386.lnk
ORIGINAL_INCLUDE_DIRS = src $(RECON)/include
include $(RECON_TOOL_DIR)/recon-tool.mk

build/wcc386.lnk: Makefile $(ORIGINAL_SOURCES)
	@mkdir -p $(@D)
	printf '%s\n' 'format windows nt' 'runtime console' 'debug all' 'option map=build\wcc386.map' 'name build\WCC386.EXE' $(foreach object,$(ORIGINAL_OBJECTS),'file $(subst /,\,$(object))') 'libpath $(subst /,\,$(ORIGINAL_TOOLCHAIN_ROOT)/lib386)' 'libpath $(subst /,\,$(ORIGINAL_TOOLCHAIN_ROOT)/lib386/nt)' 'library clib3r' 'library nt' > $@

.PHONY: original paired
original: win32
paired: cases $(RECON_TOOL)
	$(RECON_TOOL) run cases/0000-help --capture cases/0000-help/capture.bin --original-report cases/0000-help/original-report.json --report build/native-frontier.json

# CF currently needs its explicit profile argument when projecting the registry.
# Both discovery and comparison stay in the shared Rust tool.
include $(RECON_TOOL_DIR)/../graph-audit/graph-audit.mk
.PHONY: audit
audit: cases original $(GRAPH_AUDIT_BIN)
	$(RECON_TOOL) registry-export cases/0000-help --image-base 0 --cf-profile cf-profile.json --policy policy-data.json --metadata-output build/graph-metadata.json --runtime-output .build/runtime-original.json --output .build/functions.json
	printf '[]\n' > build/rebuilt-runtime.json
	"$(GRAPH_AUDIT_BIN)" --profile cl --discovery cfg --repo $(abspath ../../..) --project . --config .build/functions.json --runtime .build/runtime-original.json --rebuilt-runtime build/rebuilt-runtime.json --metadata build/graph-metadata.json --rebuilt $(ORIGINAL_OUTPUT) --map $(ORIGINAL_MAP) --map-format watcom10 --check-ownership --check-extents --check-ret --export-instructions --report build/memory-graph.json -q
