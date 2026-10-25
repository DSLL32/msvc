RECON_TOOL_DIR = $(abspath ../../../tooling/recon-tool)
RECON = ../../../lib/recon
NATIVE_CC = clang
NATIVE_CXX = clang++
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -I$(RECON)/include -I$(RECON)/native
NATIVE_CFLAGS = -std=gnu89 -fms-extensions -fasm-blocks
BINARY_SOURCES = $(wildcard src/*.c)
BINARY_SUPPORT_SOURCES = $(wildcard $(RECON)/src/*.c)

ORIGINAL_TOOLCHAIN = 1.0-nt
ORIGINAL_SOURCES = $(BINARY_SOURCES)
ORIGINAL_BUILD = .build/original
ORIGINAL_OUTPUT = $(ORIGINAL_BUILD)/CVPACK.EXE
ORIGINAL_CPPFLAGS = /Isrc /I$(RECON)/include
ORIGINAL_CFLAGS = /nologo /c /Od /G3
ORIGINAL_LDFLAGS = /nologo /subsystem:console /map:$(ORIGINAL_BUILD)/cvpack.map
ORIGINAL_DIRECT_FLAGS = --include $(abspath src) --include $(abspath $(RECON)/include)
ORIGINAL_DIRECT_DEPS = src/files.h src/image.h src/cvpack.h src/pdb.h src/types.h
ORIGINAL_INCLUDE_DIRS = src $(RECON)/include $(ORIGINAL_TOOLCHAIN_ROOT)/include

RECON_AUDIT_PROFILE = cvpack
RECON_AUDIT_REGISTRY_METADATA = 1
RECON_IMAGE_BASE = 65536
RECON_POLICY = policy-data.json
RECON_AUDIT_FLAGS = -q --discovery cfg --check-ret --check-ownership --check-extents --check-instruction-overlap --rebuilt $(ORIGINAL_OUTPUT) --map $(ORIGINAL_BUILD)/cvpack.map
RECON_AUDIT_IDENTIFY_RUNTIME = 1
include $(RECON_TOOL_DIR)/recon-tool.mk

.PHONY: audit-context
audit: audit-context
audit-context: $(ORIGINAL_OUTPUT) $(BINARY_LIBRARY) $(firstword $(CASE_TARGETS)) $(RECON_TOOL)
	python3 scripts/check-context.py $(RECON_TOOL) $(firstword $(CASE_DIRS)) $(ORIGINAL_OUTPUT) $(ORIGINAL_BUILD)/cvpack.map
