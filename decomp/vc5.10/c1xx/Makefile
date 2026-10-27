RECON_TOOL_DIR = $(abspath ../../../tooling/recon-tool)
RECON = ../../../lib/recon
NATIVE_CC = clang -std=gnu89
NATIVE_CXX = clang++
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -DRECON_CRT_PROFILE_MSVCRT_510 -I$(RECON)/include -I$(RECON)/native -Isrc -Icrt -Iharness
NATIVE_CFLAGS = -fms-extensions -fasm-blocks -Wno-ignored-attributes
NATIVE_CXXFLAGS = -std=gnu++98 -nostdinc++ -fno-exceptions -fno-rtti
BINARY_LINKER = $(NATIVE_CC)
BINARY_CRT_SOURCES = crt/startup.c
BINARY_APP_SOURCES = src/dllmain.c src/exports.c src/frontend.c \
 src/initializers.c src/output.c src/pdb-context.c src/types.c src/options.c \
 src/option-matcher.c src/checksum.c src/scanner.c \
 src/pools.cpp src/pch.cpp src/arenas.cpp src/vectors.cpp src/path.c src/packing.cpp src/driver.cpp src/frontend-init.cpp src/input.cpp src/browser.cpp src/token-init.cpp src/keywords.cpp src/pointer-array.cpp src/expression-output.cpp src/output-file.cpp src/integer-node.cpp src/ir-writer.cpp src/output-grow.cpp src/preprocessor.cpp src/global-output.cpp src/token-names.cpp
BINARY_APP_SOURCES += src/declaration.cpp src/debug-init.cpp src/parser-grammar.cpp src/parser.cpp src/parser-pending.cpp src/parser-token.cpp src/lexer.cpp src/lexer-pending.cpp src/builtin-source.cpp src/diagnostics.cpp src/numeric-token.cpp src/pragma.cpp
BINARY_APP_SOURCES += src/function-resolution.cpp src/declaration-grammar.cpp src/string-literals.cpp src/context-record.cpp
BINARY_SOURCES = $(BINARY_CRT_SOURCES) $(BINARY_APP_SOURCES)
BINARY_HARNESS_SOURCES = harness/lifecycle.c harness/minimal.c harness/input-checks.cpp harness/output-checks.cpp harness/global-output-checks.cpp
BINARY_SUPPORT_SOURCES = $(BINARY_HARNESS_SOURCES) $(addprefix $(RECON)/src/,compat.c crt.c decimal.c entry.c process.c trace.c win32.c)

ORIGINAL_TOOLCHAIN = 5.10
ORIGINAL_SOURCES = $(BINARY_SOURCES)
ORIGINAL_BUILD = build
ORIGINAL_OUTPUT = build/c1n32_l.dll
ORIGINAL_MAP = build/c1xx.map
ORIGINAL_CPPFLAGS = /Isrc /Icrt /I$(RECON)/include
ORIGINAL_CFLAGS = /nologo /c /MD /Od /W3 /GR- /GX-
ORIGINAL_LDFLAGS = /nologo /dll /nodefaultlib /entry:_DllMainCRTStartup@12 /def:src/c1xx.def
ORIGINAL_LDLIBS = $(ORIGINAL_TOOLCHAIN_ROOT)/lib/MSVCRT.LIB $(ORIGINAL_TOOLCHAIN_ROOT)/lib/KERNEL32.LIB $(ORIGINAL_TOOLCHAIN_ROOT)/lib/OLE32.LIB build/mspdb-import.lib
ORIGINAL_INCLUDE_DIRS = src crt $(RECON)/include $(ORIGINAL_TOOLCHAIN_ROOT)/include
RECON_AUDIT_PROFILE = c1xx
RECON_IMAGE_BASE = 272629760
RECON_POLICY = policy-data.json
RECON_ALIAS_MAP = symbol-aliases.json
RECON_AUDIT_REGISTRY_METADATA = 1
RECON_AUDIT_REGISTRY_RET_CHECKS = 1
RECON_AUDIT_REGISTRY_RUNTIME = 1
RECON_METADATA_REBUILT_MAP = $(ORIGINAL_MAP)
RECON_AUDIT_IDENTIFY_RUNTIME = 1
RECON_RUNTIME_INPUTS = $(ORIGINAL_TOOLCHAIN_ROOT)/lib/MSVCRT.LIB
RECON_RUNTIME_FLAGS = --library $(ORIGINAL_TOOLCHAIN_ROOT)/lib/MSVCRT.LIB
RECON_AUDIT_FLAGS = -q --discovery cfg --original ../../../toolchains/5.10/bin/C1XX.DLL \
 --rebuilt $(ORIGINAL_OUTPUT) --map $(ORIGINAL_MAP) \
 --export-instructions --check-imports --resolve-register-imports --check-references \
 --check-ownership --check-extents --include-evidenced-runtime --check-accepted-extents
include $(RECON_TOOL_DIR)/recon-tool.mk

# Preserve canonical Win32 names; only genuine target translation units get the
# native aliases. Case glue and shared runtime retain their normal identifiers.
$(filter .build/pic/src/%,$(BINARY_OBJECTS)): NATIVE_CPPFLAGS += -Dmain=c1xx_main -Dscan_internal_switches=initialize_intermediate_files

build/mspdb-import.lib: harness/mspdb-import.def | $(RECON_TOOL)
	@mkdir -p $(@D)
	$(RECON_TOOL) original tool 5.10 LIB.EXE -- /nologo /machine:i386 /def:$< /out:$@

# Explicit original capture only; native fixtures never build or run this EXE.
.PHONY: original-host
original-host: build/host/c1xx-host.exe
build/host/original-host.obj: harness/original-host.c $(RECON)/include/recon.h Makefile | $(RECON_TOOL)
	@mkdir -p $(@D)
	$(ORIGINAL_CC) /nologo /c /MD /Od /I$(RECON)/include /Fo$@ $<
build/host/c1xx-host.exe: build/host/original-host.obj
	$(ORIGINAL_LINK) /nologo /subsystem:console /out:$@ $< $(ORIGINAL_TOOLCHAIN_ROOT)/lib/MSVCRT.LIB $(ORIGINAL_TOOLCHAIN_ROOT)/lib/KERNEL32.LIB

# Only helpers proven inlined into the original initializer may inline.
build/obj/src/parser-token.cpp.obj build/obj/src/frontend-init.cpp.obj build/obj/src/input.cpp.obj build/obj/src/browser.cpp.obj build/obj/src/debug-init.cpp.obj build/obj/src/parser.cpp.obj build/obj/src/lexer.cpp.obj build/obj/src/function-resolution.cpp.obj build/obj/src/declaration.cpp.obj: ORIGINAL_CFLAGS += /Ob1

# This target-specific generator expands the authenticated C++ parser's yacc
# compression into logical shift/goto/reduction/recovery relations.
src/parser-grammar.cpp src/parser-grammar.h &: scripts/parser-grammar.py ../../../toolchains/5.10/bin/C1XX.DLL
	python3 scripts/parser-grammar.py ../../../toolchains/5.10/bin/C1XX.DLL --header src/parser-grammar.h --source src/parser-grammar.cpp

src/declaration-grammar.cpp src/declaration-grammar.h &: scripts/declaration-grammar.py ../../../toolchains/5.10/bin/C1XX.DLL
	python3 scripts/declaration-grammar.py ../../../toolchains/5.10/bin/C1XX.DLL --header src/declaration-grammar.h --source src/declaration-grammar.cpp

# All selected RETs except the one original boundary-fallthrough import role
# are checked by shared graph-audit. This finite SDK stdcall0 residual closes
# that role while preserving the full graph/import/extent selection.
.PHONY: c1xx-com-ret-check
audit: c1xx-com-ret-check
c1xx-com-ret-check: recon-graph-audit
	python3 harness/check-com-teardown.py --report $(RECON_AUDIT_REPORT) --config $(RECON_REGISTRY) --metadata $(RECON_AUDIT_METADATA) --ret-checks $(RECON_AUDIT_RET_CHECKS) --original ../../../toolchains/5.10/bin/C1XX.DLL --rebuilt $(ORIGINAL_OUTPUT) --sdk $(ORIGINAL_TOOLCHAIN_ROOT)/include/OBJBASE.H --output build/com-teardown-ret.json
