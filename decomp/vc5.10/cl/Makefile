PYTHON ?= python3
RECON_TOOL_DIR = $(abspath ../../../tooling/recon-tool)
RECON = ../../../lib/recon
NATIVE_CC = clang
NATIVE_CPPFLAGS = -DRECON_NATIVE_TRACE -DRECON_NATIVE_TEST -DRECON_DEFAULT_FMODE=0 -DRECON_CRT_PROFILE_MSVCRT_510 -I$(RECON)/include -I$(RECON)/native -Isrc
NATIVE_CFLAGS = -std=gnu89 -fms-extensions -fasm-blocks
BINARY_SOURCES = $(sort $(wildcard src/*.c))
BINARY_SUPPORT_SOURCES = scripts/native-host.c scripts/case-host.c scripts/provider-host.c scripts/dependency-host.c scripts/link-host.c scripts/mre-host.c scripts/mre-provider.c $(addprefix $(RECON)/src/,compat.c crt.c decimal.c entry.c process.c trace.c win32.c)

ORIGINAL_TOOLCHAIN = 5.10
ORIGINAL_SOURCES = $(BINARY_SOURCES)
ORIGINAL_BUILD = build
ORIGINAL_OUTPUT = build/cl.exe
ORIGINAL_CPPFLAGS = /Isrc /I$(RECON)/include
ORIGINAL_CFLAGS = /nologo /c /Od /G3 /MD
ORIGINAL_LDFLAGS = /nologo /subsystem:console /map:build/cl.map
ORIGINAL_LDLIBS = $(ORIGINAL_TOOLCHAIN_ROOT)/lib/SETARGV.OBJ $(ORIGINAL_TOOLCHAIN_ROOT)/lib/USER32.LIB build/messages.res build/mspdb50.lib
ORIGINAL_INCLUDE_DIRS = src $(RECON)/include $(ORIGINAL_TOOLCHAIN_ROOT)/include

RECON_AUDIT_PROFILE = cl
RECON_IMAGE_BASE = 4194304
RECON_POLICY = policy-data.json
RECON_AUDIT_REGISTRY_METADATA = 1
RECON_AUDIT_REGISTRY_RET_CHECKS = 1
RECON_AUDIT_REGISTRY_RUNTIME = 1
RECON_METADATA_REBUILT_MAP = build/cl.map
RECON_AUDIT_FLAGS = -q --discovery cfg --check-references --check-ownership --check-extents --include-evidenced-runtime --check-accepted-extents
RECON_AUDIT_IDENTIFY_RUNTIME = 1
RECON_RUNTIME_FLAGS = --library MSVCRT.LIB --object SETARGV.OBJ
RECON_RUNTIME_INPUTS = $(ORIGINAL_TOOLCHAIN_ROOT)/lib/MSVCRT.LIB $(ORIGINAL_TOOLCHAIN_ROOT)/lib/SETARGV.OBJ
include $(RECON_TOOL_DIR)/recon-tool.mk


build/messages.res: src/messages.rc | $(RECON_TOOL)
	@mkdir -p $(@D)
	$(RECON_TOOL) original tool 5.10 RC.EXE -- /r /fo$@ $<
build/mspdb50.lib: scripts/mspdb50.def
	@mkdir -p $(@D)
	llvm-dlltool -m i386 -d $< -l $@

# Preserve the qualified-import scope; the complete graph/RET pass stays above.
QUALIFIED_IMPORT_CONFIG = $(BINARY_BUILD)/functions-qualified-imports.json
QUALIFIED_IMPORT_REPORT = build/qualified-imports.json
$(QUALIFIED_IMPORT_CONFIG): binary.c $(RECON_POLICY) $(RECON_TOOL) $(ORIGINAL_OUTPUT) $(RECON_METADATA_REBUILT_MAP) | $(firstword $(CASE_TARGETS))
	$(RECON_TOOL) registry-export $(firstword $(CASE_DIRS)) --image-base $(RECON_IMAGE_BASE) --policy $(RECON_POLICY) --accepted-scope qualified-imports --output $@
.PHONY: qualified-imports
qualified-imports: $(QUALIFIED_IMPORT_CONFIG) $(RECON_AUDIT_METADATA) $(RECON_AUDIT_RUNTIME) $(RECON_AUDIT_ORIGINAL_RUNTIME) $(GRAPH_AUDIT_BIN) scripts/check-import-sites.py
	"$(GRAPH_AUDIT_BIN)" --profile cl --project . --repo $(RECON_REPO_ROOT) --config $(QUALIFIED_IMPORT_CONFIG) --metadata $(RECON_AUDIT_METADATA) -q --discovery cfg --check-imports --resolve-register-imports --include-evidenced-runtime --runtime $(RECON_AUDIT_ORIGINAL_RUNTIME) --rebuilt-runtime $(RECON_AUDIT_RUNTIME) --report $(QUALIFIED_IMPORT_REPORT)
	$(PYTHON) -E scripts/check-import-sites.py $(QUALIFIED_IMPORT_REPORT)
audit: qualified-imports
