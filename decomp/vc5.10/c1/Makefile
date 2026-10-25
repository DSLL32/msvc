RECON_TOOL_DIR = $(abspath ../../../tooling/recon-tool)
RECON = ../../../lib/recon
NATIVE_CC = clang -std=gnu89
NATIVE_CXX = clang++
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -DRECON_CRT_PROFILE_MSVCRT_510 -Isrc -Icrt -Iharness -I$(RECON)/include -I$(RECON)/native
NATIVE_CFLAGS = -fms-extensions -fasm-blocks -Wno-ignored-attributes
NATIVE_CXXFLAGS = -std=gnu++98 -nostdinc++ -fno-exceptions -fno-rtti
BINARY_LINKER = $(NATIVE_CC)
BINARY_SOURCES = src/dllmain.c src/exports.c crt/startup.c src/pass.c src/compiler-state.c src/initializers.c src/buffers.c src/pdb.c src/pools.cpp src/pch-state.cpp src/main.c src/work-buffers.c src/options.c src/arenas.cpp src/command-line.c src/checksums.c src/scanner.c src/option-matcher.c src/packing.cpp
BINARY_SUPPORT_SOURCES = harness/dll-cases.c $(addprefix $(RECON)/src/,compat.c crt.c decimal.c entry.c process.c trace.c win32.c)
ORIGINAL_TOOLCHAIN = 5.10
ORIGINAL_SOURCES = $(BINARY_SOURCES)
ORIGINAL_BUILD = build
ORIGINAL_OUTPUT = build/c1n32_l.dll
ORIGINAL_MAP = build/c1.map
ORIGINAL_CPPFLAGS = /Isrc /Icrt /I$(RECON)/include
ORIGINAL_CFLAGS = /nologo /c /Od /MD /G5 /GR- /GX-
ORIGINAL_LDFLAGS = /nologo /dll /nodefaultlib /entry:_DllMainCRTStartup@12 /def:src/c1.def
ORIGINAL_LDLIBS = build/mspdb50.lib $(ORIGINAL_TOOLCHAIN_ROOT)/lib/MSVCRT.LIB $(ORIGINAL_TOOLCHAIN_ROOT)/lib/KERNEL32.LIB
ORIGINAL_INCLUDE_DIRS = src crt $(RECON)/include $(ORIGINAL_TOOLCHAIN_ROOT)/include

# The registry owns code ranges/status; this file contains finite graph policy.
RECON_AUDIT_PROFILE = cl
RECON_IMAGE_BASE = 274726912
RECON_POLICY = policy-data.json
RECON_AUDIT_REGISTRY_METADATA = 1
RECON_AUDIT_REGISTRY_RUNTIME = 1
RECON_METADATA_REBUILT_MAP = $(ORIGINAL_MAP)
RECON_AUDIT_IDENTIFY_RUNTIME = 1
RECON_RUNTIME_INPUTS = $(ORIGINAL_TOOLCHAIN_ROOT)/lib/MSVCRT.LIB
RECON_RUNTIME_FLAGS = --library $(ORIGINAL_TOOLCHAIN_ROOT)/lib/MSVCRT.LIB
RECON_AUDIT_FLAGS = -q --discovery cfg --original $(abspath ../../../toolchains/5.10/bin/C1.DLL) --rebuilt $(abspath build/c1n32_l.dll) --map $(abspath build/c1.map) --check-imports --check-ret --check-ownership --check-extents --resolve-register-imports

include $(RECON_TOOL_DIR)/recon-tool.mk

$(filter .build/pic/src/%,$(BINARY_OBJECTS)): NATIVE_CPPFLAGS += -Dmain=c1_main

build/mspdb50.lib: src/mspdb50.def | $(RECON_TOOL)
	@mkdir -p $(@D)
	$(RECON_TOOL) original tool 5.10 LIB.EXE -- /nologo /def:$< /machine:IX86 /out:$@

# Explicit original-only host; native fixtures never depend on this target.
.PHONY: original-host
original-host: build/c1-host.exe
build/c1-host.obj: harness/win32-host.c
	@mkdir -p $(@D)
	$(RECON_TOOL) original tool 5.10 CL.EXE -- /nologo /c /Od /MD /G5 /I$(RECON)/include /Fo$@ $<
build/c1-host.exe: build/c1-host.obj
	$(RECON_TOOL) original tool 5.10 LINK.EXE -- /nologo /out:$@ $< $(ORIGINAL_TOOLCHAIN_ROOT)/lib/MSVCRT.LIB $(ORIGINAL_TOOLCHAIN_ROOT)/lib/KERNEL32.LIB
