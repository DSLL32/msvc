PYTHON ?= python3
TOOL = $(PYTHON) scripts/wine-tool.py
TC = ../../../toolchains/5.10
RECON = ../../../lib/recon
.DEFAULT_GOAL := native
.PHONY: all inspect native original fixtures check audit probe-help probe-minimal probe-rich
all: native
include ../../../tooling/graph-audit/graph-audit.mk
inspect:
	$(PYTHON) scripts/inspect.py
build:
	mkdir -p $@
build/%.obj: src/%.c src/rc.h | build
	$(TOOL) CL.EXE /nologo /c /MD /Od /G3 /W3 /I$(RECON)/include /Fobuild/$*.obj $<
build/rc.obj: src/rc.c src/rc.h | build
	$(TOOL) CL.EXE /nologo /c /MD /O2 /Oy- /G3 /W3 /I$(RECON)/include /Fo$@ $<
build/rcdll.lib: src/rcdll.def scripts/import-library.py scripts/binary.py | build
	$(PYTHON) scripts/import-library.py
build/rc.exe: build/rc.obj build/startup.obj build/rcdll.lib
	$(TOOL) LINK.EXE /nologo /subsystem:console /entry:mainCRTStartup /map:build/rc.map /out:$@ $^ $(TC)/lib/KERNEL32.LIB
original: build/rc.exe
NATIVE_CC ?= clang
NATIVE_FLAGS = -m32 -std=gnu89 -fms-extensions -Wno-ignored-attributes -O0 -g -DRECON_NATIVE_TEST -I$(RECON)/include -I$(RECON)/native -Isrc
NATIVE_HEADERS = $(wildcard src/*.h $(RECON)/include/*.h $(RECON)/native/*.h $(RECON)/native/sys/*.h)
NATIVE_SHIMS = $(filter-out $(RECON)/src/trace.c,$(wildcard $(RECON)/src/*.c))
NATIVE_OBJECTS = build/native/rc.o build/native/startup.o
build/native:
	mkdir -p $@
build/native/%.o: src/%.c $(NATIVE_HEADERS) | build/native
	$(NATIVE_CC) $(NATIVE_FLAGS) -Dmain=rc_main -finstrument-functions -c $< -o $@
build/native/rc: $(NATIVE_OBJECTS) scripts/native-host.c scripts/native-boundary.c scripts/native-fixture.h $(NATIVE_SHIMS) $(RECON)/src/trace.c scripts/native-build.py Makefile
	$(PYTHON) scripts/native-build.py $@ -- $(NATIVE_CC) $(NATIVE_FLAGS) $(NATIVE_OBJECTS) scripts/native-host.c scripts/native-boundary.c $(NATIVE_SHIMS) $(RECON)/src/trace.c -o $@
native: build/native/rc
audit: original $(GRAPH_AUDIT_BIN)
	$(PYTHON) scripts/audit.py
build/native/check-boundary: scripts/test-boundary.c scripts/native-boundary.c scripts/native-fixture.h $(NATIVE_HEADERS) $(NATIVE_SHIMS) | build/native
	$(NATIVE_CC) $(NATIVE_FLAGS) scripts/test-boundary.c scripts/native-boundary.c $(NATIVE_SHIMS) -o $@
fixtures check: native build/native/check-boundary
	$(PYTHON) scripts/check-native.py
	$(PYTHON) scripts/check-boundary.py
	$(PYTHON) scripts/check-rich.py
	$(PYTHON) scripts/check-rich-boundary.py
probe-help: build/debug-original.exe
	@test -n "$(REFERENCE_DIR)" || (echo 'Set REFERENCE_DIR for an explicit new original capture.' >&2; exit 2)
	$(PYTHON) scripts/capture-original.py --baseline "$(REFERENCE_DIR)"
probe-minimal probe-rich:
	@echo 'Explicit original resource captures are coordinated with the RCDLL worker; fixtures use their frozen native-only references.' >&2
	@exit 2

build/debug-original.obj: scripts/debug-original.c | build
	$(TOOL) CL.EXE /nologo /c /MD /Od /G3 /W3 /Fobuild/debug-original.obj $<
build/debug-original.exe: build/debug-original.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $< $(TC)/lib/KERNEL32.LIB
