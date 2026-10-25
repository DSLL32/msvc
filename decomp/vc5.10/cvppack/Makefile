RECON_TOOL_DIR := $(abspath ../../../tooling/recon-tool)
RECON := ../../../lib/recon
NATIVE_CC := clang
NATIVE_CXX := clang++
BINARY_LINKER := clang
APP_SOURCES := src/derived-types.c \
  src/diagnostics.c \
  src/files.c \
  src/global-symbols.c \
  src/global-types.c \
  src/image.c \
  src/input.c \
  src/legacy-types.c \
  src/main.c \
  src/messages.c \
  src/modern-types.c \
  src/module-types.c \
  src/options.c \
  src/pack.c \
  src/primitive-types.c \
  src/publics.c \
  src/source-index.c \
  src/stubs.c \
  src/symbol-handlers.c \
  src/symbol-reference.c \
  src/symbol-references.c \
  src/symbol-rewrite.c \
  src/symbol-scan.c \
  src/symbol-scopes.c \
  src/type-arena.c \
  src/type-compare.c \
  src/type-hash.c \
  src/type-index.c \
  src/type-intern.c \
  src/type-lists.c \
  src/type-lookup.c \
  src/type-names.c \
  src/type-pointers.c \
  src/type-recursive-hash.c \
  src/type-recursive.c \
  src/type-references.c \
  src/types.c \
  src/writer.c \
  src/widen.cpp
# Existing shared support rules compile C only; this host bridge uses their
# C++ dispatch list until common C++ support is available. It earns no App credit.
BINARY_SOURCES := $(APP_SOURCES) harness/native-widen-factory.cpp
BINARY_SUPPORT_SOURCES := harness/native-driver.c harness/native-widen.c harness/native-pdb.c \
 $(wildcard $(RECON)/src/*.c)
NATIVE_CPPFLAGS := -DRECON_NATIVE_TEST -DRECON_NATIVE_STARTUP_CONTROLFP_53 \
 -I$(RECON)/include -I$(RECON)/native -Isrc -Icrt
NATIVE_CFLAGS := -std=gnu89 -fms-extensions -fasm-blocks
NATIVE_CXXFLAGS := -std=gnu++98 -nostdinc++ -fno-exceptions -fno-rtti

ORIGINAL_TOOLCHAIN := 5.10
ORIGINAL_SOURCES := $(APP_SOURCES)
ORIGINAL_OUTPUT := build/CVPACK.EXE
ORIGINAL_CPPFLAGS := /Isrc /Icrt /I$(RECON)/include
ORIGINAL_CFLAGS := /nologo /c /MD /Od /W3
ORIGINAL_INCLUDE_DIRS := src crt $(RECON)/include
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
