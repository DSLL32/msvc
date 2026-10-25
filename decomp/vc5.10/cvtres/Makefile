RECON_TOOL_DIR = $(abspath ../../../tooling/recon-tool)
RECON = ../../../lib/recon
NATIVE_CC = clang
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -DRECON_CRT_PROFILE_MSVCRT_510 -DRECON_DEFAULT_FMODE=0 -DRECON_NATIVE_STARTUP_CONTROLFP_53 -I$(RECON)/include -I$(RECON)/native -Isrc
NATIVE_CFLAGS = -std=gnu89 -fms-extensions -fasm-blocks
BINARY_SOURCES = $(wildcard src/*.c)
BINARY_SUPPORT_SOURCES = $(wildcard $(RECON)/src/*.c)
ORIGINAL_TOOLCHAIN = 5.10
ORIGINAL_SOURCES = $(BINARY_SOURCES)
ORIGINAL_BUILD = .build/original
ORIGINAL_OUTPUT = $(ORIGINAL_BUILD)/CVTRES.EXE
ORIGINAL_MAP = $(ORIGINAL_BUILD)/cvtres.map
ORIGINAL_CPPFLAGS = /I$(RECON)/include
ORIGINAL_CFLAGS = /nologo /c /Od /MD
ORIGINAL_LDFLAGS = /nologo /subsystem:console /incremental:no
ORIGINAL_LDLIBS = $(ORIGINAL_BUILD)/messages.res
ORIGINAL_INCLUDE_DIRS = src $(RECON)/include $(ORIGINAL_TOOLCHAIN_ROOT)/include
RECON_AUDIT_PROFILE = c2
RECON_IMAGE_BASE = 16777216
RECON_POLICY = policy-data.json
RECON_AUDIT_FLAGS = -q --discovery cfg --check-ret --check-references --check-ownership --check-extents --check-accepted-extents --check-imports --resolve-register-imports
RECON_AUDIT_REGISTRY_METADATA = 1
RECON_AUDIT_REGISTRY_RET_CHECKS = 1
RECON_AUDIT_REGISTRY_RUNTIME = 1
RECON_AUDIT_IDENTIFY_RUNTIME = 1
RECON_METADATA_REBUILT_MAP = $(ORIGINAL_MAP)
RECON_AUDIT_FLAGS += --original ../../../toolchains/5.10/bin/CVTRES.EXE --rebuilt $(ORIGINAL_OUTPUT) --map $(ORIGINAL_MAP)
RECON_RUNTIME_INPUTS = $(ORIGINAL_TOOLCHAIN_ROOT)/lib/MSVCRT.LIB
RECON_RUNTIME_FLAGS = --library $(ORIGINAL_TOOLCHAIN_ROOT)/lib/MSVCRT.LIB
include $(RECON_TOOL_DIR)/recon-tool.mk

# Encountered target message source, original build only; never a fixture dependency.
$(ORIGINAL_BUILD)/messages.res: $(wildcard src/*.rc) | $(RECON_TOOL)
	@mkdir -p $(@D)
	$(RECON_TOOL) original tool 5.10 RC.EXE -- /r /fo$@ src/resources.rc
