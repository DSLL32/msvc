PYTHON ?= python3
RECON_TOOL_DIR = $(abspath ../../../tooling/recon-tool)
RECON = ../../../lib/recon
NATIVE_CC = clang
NATIVE_CXX = clang++
BINARY_LINKER = clang
NATIVE_CPPFLAGS = -DRECON_CRT_PROFILE_MSVCRT_510 -DRECON_NATIVE_TRACE -DRECON_NATIVE_TEST -I$(RECON)/include -I$(RECON)/native
NATIVE_CFLAGS = -std=gnu89 -fms-extensions -fasm-blocks
NATIVE_CXXFLAGS = -nostdinc++ -fno-exceptions -fno-rtti
BINARY_SOURCES = src/definition-buffer.cpp src/definition-reserve.cpp src/option-stubs.c src/symbols.c src/string-storage.c src/buffers.c src/arguments.c src/reproduction.c src/memory.c src/files.c src/file-buffers.c src/driver-stubs.c src/driver.c src/initializers.c src/full-link-init.c src/comdat.c src/sections.c src/libraries.c src/input.c src/machine.c src/module-index.c src/symbol-input.c src/input-sections.c src/default-libraries.c src/comdat-input.cpp src/symbol-table.cpp src/link-driver.cpp src/options.cpp src/full-link.cpp src/full-link-stubs.cpp src/section-driver.cpp src/pass-one.cpp src/input-machine.cpp src/input-module.cpp src/object-loader.cpp harness/startup.c
BINARY_SOURCES += src/comdat-fold.cpp src/import-sections.c src/incremental-image.c src/definition-file.cpp src/archive-index.cpp src/archive-search.cpp src/input-relocations.cpp src/x86-relocations.c src/alpha-relocations.c src/alpha-fixups.cpp src/mips-relocations.c src/mips-fixups.cpp src/arm-relocations.cpp src/ppc-relocations.c src/ppc-fixups.cpp src/sh-relocations.cpp src/editor.cpp src/dumper.cpp src/dumper-short-import.cpp src/dumper-file.cpp src/dumper-sections.cpp src/dumper-legacy.cpp src/module-symbols.cpp src/module-symbol-stubs.c
BINARY_SOURCES += src/section-layout.cpp src/pass-two.cpp src/output-symbols.cpp src/debug-output.cpp
BINARY_SOURCES += src/definition-file-stubs.c
BINARY_SOURCES += src/librarian.cpp src/librarian-extract.cpp src/librarian-stubs.cpp
BINARY_SOURCES += src/vxd.cpp src/order-file.cpp src/comdat-fold-report.cpp src/debug-lines.cpp src/map-output.cpp
BINARY_SOURCES += src/import-conversion.cpp src/archive-convert.cpp
BINARY_SOURCES += src/mac-options.c src/librarian-options.cpp
BINARY_SOURCES += src/librarian-input.cpp src/librarian-process.cpp src/librarian-prepare.cpp src/librarian-machine.cpp src/definition-parser.cpp src/definition-parser-stubs.cpp src/definition-lexer.cpp src/definition-name.cpp src/definition-export.cpp src/librarian-export.cpp src/librarian-output.cpp src/librarian-thunks.cpp src/librarian-descriptor.cpp src/librarian-exp.cpp src/archive-output.cpp src/archive-build.cpp src/messages.c
BINARY_SUPPORT_SOURCES = $(addprefix $(RECON)/src/,compat.c crt.c decimal.c entry.c process.c trace.c win32.c)

ORIGINAL_TOOLCHAIN = 5.10
ORIGINAL_SOURCES = $(filter src/%,$(BINARY_SOURCES))
ORIGINAL_BUILD = build
ORIGINAL_OUTPUT = build/link.exe
ORIGINAL_MAP = build/link.map
ORIGINAL_CPPFLAGS = /Isrc /I$(RECON)/include
ORIGINAL_CFLAGS = /nologo /c /MD /Od /Z7
# Distinct pending App entries must not be folded merely because both trap.
ORIGINAL_LDFLAGS = /nologo /incremental:no /opt:noicf
ORIGINAL_LDLIBS = build/messages.res build/operator-new-alias.obj build/operator-delete-alias.obj build/mspdb50.lib $(ORIGINAL_TOOLCHAIN_ROOT)/lib/KERNEL32.LIB $(ORIGINAL_TOOLCHAIN_ROOT)/lib/IMAGEHLP.LIB
ORIGINAL_INCLUDE_DIRS = src $(RECON)/include $(ORIGINAL_TOOLCHAIN_ROOT)/include

RECON_ALIAS_MAP = symbol-aliases.json
RECON_AUDIT_PROFILE = link
RECON_IMAGE_BASE = 4194304
RECON_POLICY = policy-data.json
RECON_AUDIT_REGISTRY_METADATA = 1
RECON_AUDIT_REGISTRY_RUNTIME = 1
RECON_AUDIT_REGISTRY_RET_CHECKS = 1
RECON_METADATA_REBUILT_MAP = build/link.map
RECON_AUDIT_PREREQUISITES = policy-contracts.json
RECON_AUDIT_FLAGS = --original $(RECON_REPO_ROOT)/toolchains/5.10/bin/LINK.EXE -q --discovery cfg --check-imports --resolve-register-imports --check-references --check-ownership --check-extents --check-callbacks --include-evidenced-runtime --export-instructions
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
build/mspdb50.lib: src/mspdb50.def
	@mkdir -p $(@D)
	$(RECON_TOOL) original tool 5.10 LIB.EXE -- /nologo /machine:IX86 /def:$< /out:$@

# C++ test glue uses only the existing shared CRT compatibility layer.
cases/0002-symbol-buffer-dtor/.build/case.so cases/0003-definition-buffer-cleanup/.build/case.so cases/0004-hello-dll/.build/case.so: CASE_LDFLAGS += -nostdlib++

# This authentic C++ lifetime is optimized away on normal paths in the original.
# Only this TU needs inlining before instrumentation; its real App outlines stay.
.build/pic/src/module-symbols.cpp.o: NATIVE_CXXFLAGS := $(filter-out -O0 -fno-inline -finstrument-functions,$(NATIVE_CXXFLAGS)) -O2 -finstrument-functions-after-inlining
build/obj/src/module-symbols.cpp.obj: ORIGINAL_CFLAGS += /O2 /GX

.PHONY: check-symbol-eh
audit: check-symbol-eh
check-symbol-eh: recon-graph-audit
	$(PYTHON) -E harness/check-symbol-eh.py --report $(RECON_AUDIT_REPORT) --original ../../../toolchains/5.10/bin/LINK.EXE --rebuilt $(ORIGINAL_OUTPUT)

# The canonical inline cleanup sees Set; callers retain the same body with
# Set only declared. Keep its selected COMDAT first in both final links.
.build/pic/src/definition-buffer.cpp.o: NATIVE_CXXFLAGS := $(filter-out -O0 -fno-inline -finstrument-functions,$(NATIVE_CXXFLAGS)) -O2 -finstrument-functions-after-inlining
build/obj/src/definition-buffer.cpp.obj: ORIGINAL_CFLAGS += /O2 /Ob2 /GX
# The bounded DIRECT qualifies these real COMDAT bodies before their ordinary
# caller is reconstructed. Keep their original-build counterparts available.
ORIGINAL_LDFLAGS += '/include:??1?$$DefinitionStorage@E@@QAE@XZ' '/include:?Set@?$$DefinitionStorage@E@@QAEHPAE0@Z'

# The definition-file owner has the same genuine optimized C++ lifetime.
.build/pic/src/definition-file.cpp.o: NATIVE_CXXFLAGS := $(filter-out -O0 -fno-inline -finstrument-functions,$(NATIVE_CXXFLAGS)) -O2 -finstrument-functions-after-inlining
build/obj/src/definition-file.cpp.obj: ORIGINAL_CFLAGS += /O2 /GX

# These real operations inline Set while retaining the separately defined Grow.
.build/pic/src/definition-reserve.cpp.o: NATIVE_CXXFLAGS := $(filter-out -O0 -fno-inline -finstrument-functions,$(NATIVE_CXXFLAGS)) -O2 -finstrument-functions-after-inlining
build/obj/src/definition-reserve.cpp.obj: ORIGINAL_CFLAGS += /O2 /Ob2

# The specialized library owners have genuine inline construction/base cleanup.
build/obj/src/librarian.cpp.obj: ORIGINAL_CFLAGS += /O2 /Ob1

build/messages.res: src/messages.rc | $(RECON_TOOL)
	@mkdir -p $(@D)
	$(RECON_TOOL) original tool 5.10 RC.EXE -- /r /fo$@ $<

# Lazy import payload and constructor-failure cleanup retain their real roles.
.build/pic/src/import-conversion.cpp.o: NATIVE_CXXFLAGS := $(filter-out -O0 -fno-inline -finstrument-functions,$(NATIVE_CXXFLAGS)) -O2 -finstrument-functions-after-inlining
build/obj/src/import-conversion.cpp.obj: ORIGINAL_CFLAGS += /O2 /Ob2 /GX

# Converted-member serialization owns the same genuine 20-byte scoped buffer.
.build/pic/src/archive-convert.cpp.o: NATIVE_CXXFLAGS := $(filter-out -O0 -fno-inline -finstrument-functions,$(NATIVE_CXXFLAGS)) -O2 -finstrument-functions-after-inlining
build/obj/src/archive-convert.cpp.obj: ORIGINAL_CFLAGS += /O2 /GX

# This real scoped temporary retains the existing qualified cleanup outline.
.build/pic/src/comdat-fold-report.cpp.o: NATIVE_CXXFLAGS := $(filter-out -O0 -fno-inline -finstrument-functions,$(NATIVE_CXXFLAGS)) -O2 -finstrument-functions-after-inlining
build/obj/src/comdat-fold-report.cpp.obj: ORIGINAL_CFLAGS += /O2 /GX

# The real per-file new expression has inline lifetime and Win32 EH cleanup.
.build/pic/src/dumper-file.cpp.o: NATIVE_CXXFLAGS := $(filter-out -O0 -fno-inline -finstrument-functions,$(NATIVE_CXXFLAGS)) -O2 -finstrument-functions-after-inlining
build/obj/src/dumper-file.cpp.obj: ORIGINAL_CFLAGS += /O2 /Ob2 /GX

# This short-import decoder owns the same qualified scoped 20-byte buffer.
.build/pic/src/dumper-short-import.cpp.o: NATIVE_CXXFLAGS := $(filter-out -O0 -fno-inline -finstrument-functions,$(NATIVE_CXXFLAGS)) -O2 -finstrument-functions-after-inlining
build/obj/src/dumper-short-import.cpp.obj: ORIGINAL_CFLAGS += /O2 /Ob2 /GX
