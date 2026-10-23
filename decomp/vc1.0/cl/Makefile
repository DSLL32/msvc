PYTHON ?= python3
TOOL = $(PYTHON) scripts/wine-tool.py
TC = ../../../toolchains/1.0-nt
RECON = ../../../lib/recon

.PHONY: all clean check prefix replay audit probe reference-build
all: native

include ../../../tooling/graph-audit/graph-audit.mk
GRAPH_AUDIT_FLAGS = --discovery cfg --check-ret --check-ownership --check-extents
GRAPH_AUDIT_INPUTS ?=

prefix:
	$(PYTHON) scripts/wine-tool.py --init

runtime.json: functions.json scripts/identify-runtime.py scripts/binary.py $(TC)/bin/CL.EXE $(TC)/lib/LIBC.LIB $(TC)/lib/SETARGV.OBJ
	$(PYTHON) scripts/identify-runtime.py

build/runtime.json: build/cl.exe functions.json scripts/identify-runtime.py scripts/binary.py
	$(PYTHON) scripts/identify-runtime.py build/cl.exe $@

# Original toolchain compilation and static graph validation are explicit.
reference-build: build/cl.exe

audit: build/runtime.json runtime.json $(GRAPH_AUDIT_BIN)
	"$(GRAPH_AUDIT_BIN)" --profile cl --repo "$(abspath ../../..)" --project "$(CURDIR)" $(GRAPH_AUDIT_FLAGS) $(GRAPH_AUDIT_INPUTS)


build:
	mkdir -p $@

build/cl.obj: src/cl.c src/cl.h | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /I$(RECON)/include /Fobuild/cl.obj src/cl.c

build/%.obj: src/%.c src/cl.h | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /I$(RECON)/include /Fobuild/$*.obj $<

build/cl.exe: build/cl.obj build/stubs.obj build/data.obj build/runtime.obj build/driver.obj build/remaining.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /map:build/cl.map /out:$@ $^ $(TC)/lib/SETARGV.OBJ


build/gui-check.exe: build/gui-check.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $< $(TC)/lib/USER32.LIB $(TC)/lib/GDI32.LIB

build/trace-run.exe: build/trace-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<

build/pass-proxy.exe build/fail-pass.exe build/wait-pass.exe &: build/pass-helper.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:build/pass-proxy.exe $<
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:build/fail-pass.exe $<
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:build/wait-pass.exe $<

probe: build/trace-run.exe build/pass-proxy.exe build/fail-pass.exe build/wait-pass.exe
	$(PYTHON) scripts/probe-cases.py

clean:
	rm -rf build

# Native fixture execution never depends on Wine or a PE executable.
.PHONY: native native-trace native-check native-trace-check native-reject-check
native-reject-check: native-trace
	$(PYTHON) scripts/check-native-negative.py
check replay: native-trace-check
native: build/native/CL.EXE
native-trace: build/native-trace/CL.EXE
native-check: native
	$(PYTHON) scripts/check-native.py
native-trace-check: native-trace
	$(PYTHON) scripts/check-native.py --trace
NATIVE_CC ?= clang
NATIVE_FLAGS = -m32 -std=gnu89 -fms-extensions -O0 -g -DRECON_NATIVE_TEST -I$(RECON)/include -I$(RECON)/native
NATIVE_NAMES = cl stubs data runtime driver remaining
NATIVE_HEADERS = $(wildcard src/*.h) $(wildcard $(RECON)/include/*.h) $(wildcard $(RECON)/native/*.h) $(wildcard $(RECON)/native/sys/*.h)
NATIVE_SHIMS = $(filter-out $(RECON)/src/trace.c,$(wildcard $(RECON)/src/*.c))
build/native build/native-trace:
	mkdir -p $@
build/native/%.o: src/%.c $(NATIVE_HEADERS) | build/native
	$(NATIVE_CC) $(NATIVE_FLAGS) -c $< -o $@
build/native-trace/%.o: src/%.c $(NATIVE_HEADERS) | build/native-trace
	$(NATIVE_CC) $(NATIVE_FLAGS) -finstrument-functions -c $< -o $@
build/native-trace/runtime.o: src/runtime.c $(NATIVE_HEADERS) | build/native-trace
	$(NATIVE_CC) $(NATIVE_FLAGS) -c $< -o $@
build/native/CL.EXE: $(addprefix build/native/,$(addsuffix .o,$(NATIVE_NAMES))) $(NATIVE_SHIMS) Makefile
	$(NATIVE_CC) $(NATIVE_FLAGS) $(filter %.o,$^) $(NATIVE_SHIMS) -o $@
build/native-trace/CL.EXE: $(addprefix build/native-trace/,$(addsuffix .o,$(NATIVE_NAMES))) $(NATIVE_SHIMS) $(RECON)/src/trace.c Makefile
	$(NATIVE_CC) $(NATIVE_FLAGS) $(filter %.o,$^) $(NATIVE_SHIMS) $(RECON)/src/trace.c -o $@

# Original-only capture helper; this target is never a fixture dependency.
build/capture-run.exe: build/capture-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<
