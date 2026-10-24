PYTHON ?= python3
TOOL = $(PYTHON) scripts/wine-tool.py
TC = ../../../toolchains/5.10
RECON = ../../../lib/recon
NATIVE_CC ?= clang
LLVM_DLLTOOL ?= llvm-dlltool
# Freeze the shared release actually compiled, independently of concurrent WIP.
RECON_SHARED_REVISION ?= 2265dfaad
export RECON_SHARED_REVISION
NATIVE_FLAGS = -m32 -std=gnu89 -fms-extensions -fasm-blocks -O0 -g -DRECON_NATIVE_TEST -DRECON_NATIVE_ENTRY_CONTROLFP_53 -DRECON_DEFAULT_FMODE=0 -DRECON_CRT_PROFILE_MSVCRT_510 -I$(RECON)/include -I$(RECON)/native
NATIVE_SHIMS = $(filter-out $(RECON)/src/trace.c,$(wildcard $(RECON)/src/*.c))
SOURCES = $(wildcard src/*.c)
NAMES = $(basename $(notdir $(SOURCES)))
SOURCE_HEADERS = $(wildcard src/*.h)
RESOURCE_PROOFS = $(wildcard cases/*/resources.json)
# Native headers/shims come from the pinned Git release, never mutable WIP.
NATIVE_RELEASE = build/shared-$(RECON_SHARED_REVISION).stamp
ORIGINAL_HEADERS = $(SOURCE_HEADERS) $(RECON)/include/recon.h

.PHONY: all inspect check native native-trace original audit mask fixtures
all: native
include ../../../tooling/graph-audit/graph-audit.mk

inspect:
	$(PYTHON) scripts/inspect.py --check
native: build/native/CL.EXE build/resources.bin
native-trace: build/native-trace/CL.EXE build/resources.bin
original: build/cl.exe
build build/native build/native-trace:
	mkdir -p $@
$(NATIVE_RELEASE): | build
	git rev-parse --verify $(RECON_SHARED_REVISION) > $@
build/native/%.o: src/%.c $(SOURCE_HEADERS) $(NATIVE_RELEASE) Makefile scripts/native-build.py scripts/build_inputs.py | build/native
	$(PYTHON) scripts/native-build.py $(NATIVE_CC) $(NATIVE_FLAGS) -c $< -o $@
build/native-trace/%.o: src/%.c $(SOURCE_HEADERS) $(NATIVE_RELEASE) Makefile scripts/native-build.py scripts/build_inputs.py | build/native-trace
	$(PYTHON) scripts/native-build.py $(NATIVE_CC) $(NATIVE_FLAGS) -finstrument-functions -c $< -o $@
build/native/CL.EXE: $(addprefix build/native/,$(addsuffix .o,$(NAMES))) $(NATIVE_RELEASE) Makefile scripts/native-build.py scripts/build_inputs.py scripts/native-host.c scripts/mre-host.c scripts/dependency-host.c scripts/link-host.c
	$(PYTHON) scripts/native-build.py $(NATIVE_CC) $(NATIVE_FLAGS) $(filter %.o,$^) scripts/native-host.c scripts/mre-host.c scripts/dependency-host.c scripts/link-host.c $(NATIVE_SHIMS) -Wl,--wrap=main -o $@
build/native-trace/CL.EXE: $(addprefix build/native-trace/,$(addsuffix .o,$(NAMES))) $(NATIVE_RELEASE) Makefile scripts/native-build.py scripts/build_inputs.py scripts/native-host.c scripts/mre-host.c scripts/dependency-host.c scripts/link-host.c
	$(PYTHON) scripts/native-build.py $(NATIVE_CC) $(NATIVE_FLAGS) $(filter %.o,$^) scripts/native-host.c scripts/mre-host.c scripts/dependency-host.c scripts/link-host.c $(NATIVE_SHIMS) $(RECON)/src/trace.c -Wl,--wrap=main -o $@
build/%.obj: src/%.c $(ORIGINAL_HEADERS) Makefile | build
	$(TOOL) CL.EXE /nologo /c /Od /G3 /MD /I$(RECON)/include /Fobuild/$*.obj $<
build/resources.bin: src/messages.rc $(RESOURCE_PROOFS) scripts/resources.py ../../../lib/recon/resources.py | build
	$(PYTHON) scripts/resources.py --prepare $@
build/messages.res: src/messages.rc | build
	$(TOOL) RC.EXE /r /fo$@ $<
build/mspdb50.lib: scripts/mspdb50.def scripts/mre_imports.py cases/minimal-c/mre-imports.json | build
	$(PYTHON) scripts/mre_imports.py --build-import-library $@ --dlltool $(LLVM_DLLTOOL)
build/cl.exe: $(addprefix build/,$(addsuffix .obj,$(NAMES))) $(TC)/lib/SETARGV.OBJ $(TC)/lib/USER32.LIB build/messages.res build/mspdb50.lib
	$(TOOL) LINK.EXE /nologo /subsystem:console /map:build/cl.map /out:$@ $^
	$(PYTHON) scripts/resources.py --verify-rebuilt
runtime.json: scripts/identify-runtime.py scripts/binary.py cases/help/import-support.json $(TC)/bin/CL.EXE $(TC)/lib/MSVCRT.LIB $(TC)/lib/SETARGV.OBJ
	$(PYTHON) scripts/identify-runtime.py --output $@
build/runtime.json: build/cl.exe scripts/identify-runtime.py scripts/binary.py cases/help/import-support.json
	$(PYTHON) scripts/identify-runtime.py --rebuilt --output $@
graph-metadata.json: runtime.json build/runtime.json scripts/runtime-metadata.py cases/minimal-c/getprocaddress-preservation.json
	$(PYTHON) scripts/runtime-metadata.py
audit: build/cl.exe build/runtime.json functions.json runtime.json graph-metadata.json $(GRAPH_AUDIT_BIN)
	@$(PYTHON) scripts/audit.py

mask: functions.json | build
	$(PYTHON) scripts/mask.py
check: inspect native
	$(PYTHON) scripts/check-scaffold.py
	$(PYTHON) scripts/check-acceptance.py
	$(PYTHON) scripts/check-rules.py
	$(PYTHON) scripts/check-input-kinds.py
fixtures:
	$(PYTHON) scripts/fixtures.py

# Explicit original-only debugger, never a native fixture prerequisite.
build/reference-debugger.obj: scripts/reference-debugger.c scripts/dependency-observer.inc scripts/clock-observer.inc scripts/mre-observer.inc scripts/mre-file-observer.inc scripts/export-reader.h | build
	$(TOOL) CL.EXE /nologo /c /W3 /Od /MT /Fobuild/reference-debugger.obj $<
build/reference-debugger.exe: build/reference-debugger.obj scripts/helper-build.py
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<
	$(PYTHON) scripts/helper-build.py
