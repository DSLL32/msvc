PYTHON ?= python3
RECON_TOOL_DIR = $(abspath ../../../tooling/recon-tool)
RECON = ../../../lib/recon
NATIVE_CC = clang
NATIVE_CXX = clang++
BINARY_LINKER = clang
NATIVE_CPPFLAGS = -DRECON_CRT_PROFILE_MSVCRT_510 -DRECON_NATIVE_TRACE -DRECON_NATIVE_TEST -I$(RECON)/include -I$(RECON)/native
NATIVE_CFLAGS = -std=gnu89 -fms-extensions -fasm-blocks
NATIVE_CXXFLAGS = -nostdinc++ -fno-exceptions -fno-rtti
BINARY_SOURCES = src/option-stubs.c src/symbols.c src/arguments.c src/reproduction.c src/memory.c src/files.c src/driver-stubs.c src/driver.c src/initializers.c src/full-link-init.c src/comdat.c src/sections.c src/libraries.c src/input.c src/machine.c src/module-index.c src/symbol-input.c src/link-driver.cpp src/options.cpp src/full-link.cpp src/full-link-stubs.cpp src/section-driver.cpp src/pass-one.cpp src/input-machine.cpp src/input-module.cpp src/object-loader.cpp harness/startup.c
BINARY_SUPPORT_SOURCES = $(addprefix $(RECON)/src/,compat.c crt.c decimal.c entry.c process.c trace.c win32.c)

ORIGINAL_TOOLCHAIN = 5.10
ORIGINAL_SOURCES = $(filter src/%,$(BINARY_SOURCES))
ORIGINAL_BUILD = build
ORIGINAL_OUTPUT = build/link.exe
ORIGINAL_MAP = build/link.map
ORIGINAL_CPPFLAGS = /Isrc /I$(RECON)/include
ORIGINAL_CFLAGS = /nologo /c /MD /Od /Z7
ORIGINAL_LDFLAGS = /nologo /incremental:no
ORIGINAL_LDLIBS = build/operator-new-alias.obj build/operator-delete-alias.obj $(ORIGINAL_TOOLCHAIN_ROOT)/lib/KERNEL32.LIB
ORIGINAL_INCLUDE_DIRS = src $(RECON)/include $(ORIGINAL_TOOLCHAIN_ROOT)/include

RECON_AUDIT_PROFILE = link
RECON_IMAGE_BASE = 4194304
RECON_POLICY = policy-data.json
RECON_AUDIT_REGISTRY_METADATA = 1
RECON_AUDIT_REGISTRY_RUNTIME = 1
RECON_AUDIT_REGISTRY_RET_CHECKS = 1
RECON_METADATA_REBUILT_MAP = build/link.map
RECON_AUDIT_PREREQUISITES = policy-contracts.json
RECON_AUDIT_FLAGS = --original $(RECON_REPO_ROOT)/toolchains/5.10/bin/LINK.EXE -q --discovery cfg --check-imports --check-references --check-ownership --check-extents --check-callbacks --include-evidenced-runtime
RECON_AUDIT_IDENTIFY_RUNTIME = 1
RECON_RUNTIME_INPUTS = $(ORIGINAL_TOOLCHAIN_ROOT)/lib/MSVCRT.LIB
RECON_RUNTIME_FLAGS = --library MSVCRT.LIB
include $(RECON_TOOL_DIR)/recon-tool.mk

.build/pic/src/driver.c.o: NATIVE_CPPFLAGS += -Dmain=link_main
$(filter %.cpp.obj,$(ORIGINAL_OBJECTS)): ORIGINAL_CFLAGS += /GR- /GX-
$(filter %.cpp.o,$(BINARY_OBJECTS)): NATIVE_CFLAGS := $(filter-out -std=gnu89,$(NATIVE_CFLAGS)) -std=gnu++98

build/operator-new-alias.obj: scripts/weak-import.py
	@mkdir -p $(@D)
	$(PYTHON) scripts/weak-import.py $@
build/operator-delete-alias.obj: scripts/weak-import.py
	@mkdir -p $(@D)
	$(PYTHON) scripts/weak-import.py --delete $@
