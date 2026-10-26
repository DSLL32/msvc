RECON_TOOL_DIR = $(abspath ../../../tooling/recon-tool)
RECON = ../../../lib/recon
NATIVE_CC = gcc
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -DRECON_CRT_PROFILE_WATCOM_100A -I$(RECON)/include -I$(RECON)/native -Isrc -Iharness
NATIVE_CFLAGS = -std=gnu89
BINARY_SOURCES = $(sort $(wildcard src/*.c crt/*.c))
BINARY_SUPPORT_SOURCES = harness/startup.c $(wildcard $(RECON)/src/*.c)

ORIGINAL_TOOLCHAIN = watcom10.0a
ORIGINAL_SOURCES = $(BINARY_SOURCES)
ORIGINAL_OUTPUT = build/WCC386.EXE
ORIGINAL_MAP = build/wcc386.map
ORIGINAL_CPPFLAGS = '-i=..\..\..\toolchains\watcom10.0a\h' '-i=..\..\..\toolchains\watcom10.0a\h\nt' '-i=src' '-i=crt' '-i=..\..\..\lib\recon\include'
ORIGINAL_CFLAGS = -bt=nt -3r -od -d2 -s
ORIGINAL_CC = $(RECON_TOOL) original tool watcom10.0a WCC386.EXE --
ORIGINAL_LINK = $(RECON_TOOL) original tool watcom10.0a WLINK.EXE --
ORIGINAL_COMPILE = $(ORIGINAL_CC) $(ORIGINAL_CPPFLAGS) $(ORIGINAL_CFLAGS) '-fo=$(subst /,\,$@)' '$(subst /,\,$<)'
ORIGINAL_LINK_COMMAND = $(ORIGINAL_LINK) '@build\wcc386.lnk'
ORIGINAL_LDLIBS = build/wcc386.lnk
ORIGINAL_INCLUDE_DIRS = src crt $(RECON)/include
RECON_AUDIT_PROFILE = cl
RECON_IMAGE_BASE = 0
RECON_CF_PROFILE = cf-profile.json
RECON_POLICY = build/audit-policy.json
RECON_METADATA_REBUILT_MAP = $(ORIGINAL_MAP)
RECON_AUDIT_REGISTRY_METADATA = 1
RECON_AUDIT_REGISTRY_RUNTIME = 1
RECON_DIAGNOSTIC_DIR = $(HOME)/.ai-tmp/msvc2/watcom10.0a/wcc386/help
RECON_AUDIT_REPORT = $(RECON_DIAGNOSTIC_DIR)/memory-graph.json
RECON_AUDIT_PREREQUISITES = $(ORIGINAL_OUTPUT) $(ORIGINAL_MAP) build/rebuilt-runtime.json harness/ret-checks.json scripts/check-nonlocal-tails.py
RECON_AUDIT_FLAGS = -q --discovery cfg --check-ownership --check-extents --ret-checks harness/ret-checks.json --export-instructions --rebuilt $(ORIGINAL_OUTPUT) --map $(ORIGINAL_MAP) --map-format watcom10 --rebuilt-runtime build/rebuilt-runtime.json
include $(RECON_TOOL_DIR)/recon-tool.mk

build/wcc386.lnk: Makefile $(ORIGINAL_SOURCES)
	@mkdir -p $(@D)
	printf '%s\n' 'format windows nt' 'runtime console' 'debug all' 'option map=build\wcc386.map' 'name build\WCC386.EXE' $(foreach object,$(ORIGINAL_OBJECTS),'file $(subst /,\,$(object))') 'libpath $(subst /,\,$(ORIGINAL_TOOLCHAIN_ROOT)/lib386)' 'libpath $(subst /,\,$(ORIGINAL_TOOLCHAIN_ROOT)/lib386/nt)' 'library clib3r' 'library nt' > $@

.PHONY: original paired
original: win32
paired: cases $(RECON_TOOL)
	@mkdir -p "$(RECON_DIAGNOSTIC_DIR)"
	$(RECON_TOOL) run cases/0000-help --capture cases/0000-help/capture.bin --report $(RECON_DIAGNOSTIC_DIR)/native-frontier.json

build/rebuilt-runtime.json:
	@mkdir -p $(@D)
	printf '[]\n' > $@

# NewSym/NewLeaf cold failure tails and CCusage noreturn use the finite residual;
# all ordinary returning App bodies retain strict shared RET contracts.
.PHONY: audit-nonlocal-tails
audit: audit-nonlocal-tails
audit-nonlocal-tails: $(ORIGINAL_OUTPUT) $(ORIGINAL_MAP) scripts/check-nonlocal-tails.py
	python3 scripts/check-nonlocal-tails.py

# Keep this tiny EAX-forwarding exit wrapper compact for its finite ABI residual.
build/obj/src/exit.c.obj: ORIGINAL_CFLAGS += -ox

# Update native bootstrap ownership only at the App stdio boundary.
.build/pic/src/frontend.c.o: NATIVE_CPPFLAGS += -include harness/stream-boundary.h

# Materialize only the finite object-proven alignment descriptor. This is
# semantic projection input; diagnostic reports remain in owned scratch.
build/audit-policy.json: harness/policy.json scripts/audit-policy.py build/obj/src/usage.c.obj $(ORIGINAL_OUTPUT) $(ORIGINAL_MAP)
	python3 scripts/audit-policy.py --output $@
