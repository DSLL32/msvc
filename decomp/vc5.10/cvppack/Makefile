RECON_TOOL_DIR := $(abspath ../../../tooling/recon-tool)
RECON := ../../../lib/recon
NATIVE_CC := clang
NATIVE_CXX := clang++
BINARY_LINKER := clang
BINARY_SOURCES := $(wildcard src/*.c) $(wildcard src/*.cpp) scripts/native-widen-factory.cpp
BINARY_SUPPORT_SOURCES := scripts/native-driver.c scripts/native-widen.c scripts/native-pdb.c \
 $(wildcard $(RECON)/src/*.c)
NATIVE_CPPFLAGS := -DRECON_NATIVE_TEST -DRECON_NATIVE_STARTUP_CONTROLFP_53 \
 -I$(RECON)/include -I$(RECON)/native -Isrc
NATIVE_CFLAGS := -std=gnu89 -fms-extensions -fasm-blocks
NATIVE_CXXFLAGS := -std=gnu++98 -nostdinc++ -fno-exceptions -fno-rtti

ORIGINAL_TOOLCHAIN := 5.10
ORIGINAL_SOURCES := $(wildcard src/*.c) $(wildcard src/*.cpp)
ORIGINAL_OUTPUT := build/CVPACK.EXE
ORIGINAL_CPPFLAGS := /I$(RECON)/include
ORIGINAL_CFLAGS := /nologo /c /MD /Od /W3
ORIGINAL_INCLUDE_DIRS := src $(RECON)/include
ORIGINAL_LDFLAGS := /nologo /subsystem:console /map:build/cvpack.map
ORIGINAL_LDLIBS := build/resources.res build/mspdb50.lib

RECON_AUDIT_PROFILE := cl
RECON_IMAGE_BASE := 4194304
RECON_POLICY := policy-data.json
RECON_AUDIT_REGISTRY_METADATA := 1
RECON_AUDIT_REGISTRY_RET_CHECKS := 1
RECON_AUDIT_REGISTRY_RUNTIME := 1
RECON_AUDIT_IDENTIFY_RUNTIME := 1
RECON_RUNTIME_FLAGS := --library MSVCRT.LIB
RECON_RUNTIME_INPUTS = $(ORIGINAL_TOOLCHAIN_ROOT)/lib/MSVCRT.LIB
RECON_METADATA_REBUILT_MAP := build/cvpack.map
RECON_AUDIT_FLAGS = -q --discovery cfg --check-imports --resolve-register-imports \
 --check-callbacks --check-references --check-ownership --check-extents \
 --original ../../../toolchains/5.10/bin/CVPACK.EXE --rebuilt $(ORIGINAL_OUTPUT) --map build/cvpack.map
RECON_AUDIT_PREREQUISITES = $(ORIGINAL_OUTPUT) build/cvpack.map

include $(RECON_TOOL_DIR)/recon-tool.mk
NATIVE_CXXFLAGS := $(filter-out -std=gnu89,$(NATIVE_CXXFLAGS))

build/resources.res: src/cvpack.rc | $(RECON_TOOL)
	@mkdir -p $(@D)
	$(RECON_TOOL) original tool 5.10 RC.EXE -- /r /fo$@ $<
build/mspdb50.lib: src/mspdb50.def | $(RECON_TOOL)
	@mkdir -p $(@D)
	$(RECON_TOOL) original tool 5.10 LIB.EXE -- /nologo /machine:IX86 /def:$< /out:$@
build/cvpack.map: $(ORIGINAL_OUTPUT)
	@test -f $@

$(RECON_REGISTRY_OUTPUTS): abi-policy.json
# Scalar dispatch tags are target-specific data, checked by the narrow case test.
audit: cases/0001-hello-debug-dll/unit
