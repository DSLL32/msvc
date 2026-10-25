PYTHON ?= python3
RECON_TOOL_DIR = $(abspath ../../../tooling/recon-tool)
RECON = ../../../lib/recon
NATIVE_CC = clang
NATIVE_CPPFLAGS = -DRECON_NATIVE_TRACE -DRECON_NATIVE_TEST -DRECON_CRT_PROFILE_MSVCRT_510 -DRECON_LEGACY_JMP_WORDS=16 -DRECON_NATIVE_SETJMP3_COUNT0 -Dmain=rcdll_command_main -I$(RECON)/include -I$(RECON)/native -Isrc -DRECON_RESOURCE_FILE='"$(abspath .build/help-resources.bin)"'
NATIVE_CFLAGS = -std=gnu89 -fms-extensions -fasm-blocks -fshort-wchar
BINARY_SOURCES = $(wildcard src/*.c)
BINARY_SUPPORT_SOURCES = $(wildcard $(RECON)/src/*.c)

ORIGINAL_TOOLCHAIN = 5.10
ORIGINAL_SOURCES = $(BINARY_SOURCES)
ORIGINAL_BUILD = .build/win32
ORIGINAL_OUTPUT = $(ORIGINAL_BUILD)/RCDLL.DLL
ORIGINAL_MAP = $(ORIGINAL_BUILD)/rcdll.map
ORIGINAL_CPPFLAGS = /Isrc /I$(RECON)/include
ORIGINAL_CFLAGS = /nologo /c /Od /MD /W3
ORIGINAL_LDFLAGS = /nologo /DLL /entry:_DllMainCRTStartup@12 /def:src/rcdll.def
ORIGINAL_LDLIBS = .build/help.res $(ORIGINAL_TOOLCHAIN_ROOT)/lib/MSVCRT.LIB
ORIGINAL_LINK_COMMAND = $(ORIGINAL_LINK) $(ORIGINAL_LDFLAGS) /map:$(ORIGINAL_MAP) /out:$(ORIGINAL_OUTPUT) $(ORIGINAL_OBJECTS) $(ORIGINAL_LDLIBS) kernel32.lib user32.lib
ORIGINAL_INCLUDE_DIRS = src $(RECON)/include $(ORIGINAL_TOOLCHAIN_ROOT)/include

RECON_AUDIT_PROFILE = link
RECON_IMAGE_BASE = 1908539392
RECON_POLICY = policy-data.json
RECON_AUDIT_FLAGS = -q --original ../../../toolchains/5.10/bin/RCDLL.DLL --rebuilt $(ORIGINAL_OUTPUT) --map $(ORIGINAL_MAP) --discovery cfg --check-ret --check-references --check-ownership --check-extents --check-accepted-extents --export-instructions
RECON_AUDIT_REGISTRY_METADATA = 1
RECON_METADATA_REBUILT_MAP = $(ORIGINAL_MAP)
RECON_AUDIT_REGISTRY_RET_CHECKS = 1
RECON_AUDIT_REGISTRY_RUNTIME = 1
RECON_AUDIT_IDENTIFY_RUNTIME = 1
RECON_RUNTIME_INPUTS = $(ORIGINAL_TOOLCHAIN_ROOT)/lib/MSVCRT.LIB
RECON_RUNTIME_FLAGS = --library $(ORIGINAL_TOOLCHAIN_ROOT)/lib/MSVCRT.LIB
include $(RECON_TOOL_DIR)/recon-tool.mk

$(BINARY_LIBRARY): | .build/help-resources.bin
.build/help-resources.bin .build/help-messages.bin &: src/help.mc scripts/prepare-resources.py $(RECON)/resources.py
	$(PYTHON) scripts/prepare-resources.py
.build/help.res: src/help.rc .build/help-messages.bin | $(RECON_TOOL)
	$(RECON_TOOL) original tool 5.10 RC.EXE -- /r /fo$@ src/help.rc
.PHONY: original
original: recon-original

.PHONY: check-seh
audit: check-seh
check-seh: recon-graph-audit
	$(PYTHON) scripts/check-seh.py --report $(RECON_AUDIT_REPORT) --policy $(RECON_POLICY) --original ../../../toolchains/5.10/bin/RCDLL.DLL --rebuilt $(ORIGINAL_OUTPUT)
