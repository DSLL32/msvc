PYTHON ?= python3
NATIVE_CC ?= clang
RECON = ../../../lib/recon
TC = ../../../toolchains/5.10
TOOL = $(PYTHON) scripts/wine-tool.py
NATIVE_FLAGS = -m32 -std=gnu89 -fms-extensions -fasm-blocks -Wno-ignored-attributes -DRECON_DEFAULT_FMODE=0 -O0 -g -DRECON_NATIVE_TEST -I$(RECON)/include -I$(RECON)/native -Isrc
NATIVE_SHIMS = $(filter-out $(RECON)/src/trace.c,$(wildcard $(RECON)/src/*.c))
PROVENANCE_NATIVE = $(wildcard src/*) scripts/native-host.c $(wildcard $(RECON)/src/*.c $(RECON)/include/*.h $(RECON)/native/*.h $(RECON)/native/sys/*.h) Makefile
PROVENANCE_ORIGINAL = $(wildcard src/*) Makefile
NATIVE_HEADERS = $(wildcard src/*.h $(RECON)/include/*.h $(RECON)/native/*.h $(RECON)/native/sys/*.h)
.DEFAULT_GOAL := native
include ../../../tooling/graph-audit/graph-audit.mk

.PHONY: all native original inspect groundwork audit check fixtures capture-original-help
all: native
native: build/native/c2
original: build/c2.exe
inspect groundwork:
	$(PYTHON) scripts/inspect.py
build build/native:
	mkdir -p $@
build/native/input-start.json: $(PROVENANCE_NATIVE) scripts/build-provenance.py | build/native
	$(PYTHON) scripts/build-provenance.py begin native
build/original/input-start.json: $(PROVENANCE_ORIGINAL) scripts/build-provenance.py | build
	$(PYTHON) scripts/build-provenance.py begin original
build/native/%.o: src/%.c $(NATIVE_HEADERS) | build/native build/native/input-start.json
	$(NATIVE_CC) $(NATIVE_FLAGS) -DRECON_NATIVE_TRACE -finstrument-functions -c $< -o $@
build/native/c2: build/native/stubs.o build/native/startup.o build/native/precision.o build/native/main.o scripts/native-host.c $(NATIVE_SHIMS) $(RECON)/src/trace.c $(NATIVE_HEADERS) Makefile build/native/input-start.json
	$(NATIVE_CC) $(NATIVE_FLAGS) $(filter %.o,$^) scripts/native-host.c $(NATIVE_SHIMS) $(RECON)/src/trace.c -o $@
	$(PYTHON) scripts/build-provenance.py finish native --binary $@
ORIGINAL_CRT_FLAGS = /MD
build/precision.obj: ORIGINAL_CRT_FLAGS = /ML /Zl
build/%.obj: src/%.c src/c2.h | build build/original/input-start.json
	$(TOOL) CL.EXE /nologo /c /Od /G3 /W3 $(ORIGINAL_CRT_FLAGS) /I$(RECON)/include /Fo$@ $<
build/c2.exe: build/stubs.obj build/startup.obj build/precision.obj build/main.obj build/original/input-start.json
	$(TOOL) LINK.EXE /nologo /subsystem:console /entry:c2_crt_startup /map:build/c2.map /out:$@ $(filter %.obj,$^) kernel32.lib
	$(PYTHON) scripts/build-provenance.py finish original --binary $@
build/debug-run.obj: scripts/debug-run.c | build
	$(TOOL) CL.EXE /nologo /c /Od /G3 /W3 /ML /I$(RECON)/include /Fo$@ $<
build/debug-run.exe: build/debug-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $^
build/runtime.json build/graph-metadata.json &: build/c2.exe build/c2.map functions.json scripts/build-index.py
	$(PYTHON) scripts/build-index.py
build/c2.map: build/c2.exe
	@test -f $@
build/trial.json: functions.json scripts/state.py | build
	$(PYTHON) -c "import sys; sys.path.insert(0,'scripts'); from state import trial_configuration; from pathlib import Path; Path('$@').write_bytes(trial_configuration(Path('functions.json').read_bytes())[1])"
audit: build/c2.exe build/runtime.json build/graph-metadata.json build/trial.json $(GRAPH_AUDIT_BIN)
	"$(GRAPH_AUDIT_BIN)" --profile cl --repo "$(abspath ../../..)" --project "$(CURDIR)" --original "$(abspath $(TC)/bin/C2.EXE)" --rebuilt "$(CURDIR)/build/c2.exe" --map "$(CURDIR)/build/c2.map" --config "$(CURDIR)/build/trial.json" --runtime "$(CURDIR)/runtime.json" --rebuilt-runtime "$(CURDIR)/build/runtime.json" --metadata "$(CURDIR)/build/graph-metadata.json" --discovery cfg --check-ret --check-imports --check-ownership --check-extents --report "$(CURDIR)/build/graph-audit.json"
check fixtures:
	@echo 'No completed native fixture yet; reconstruction scaffold is at first entry frontier.' >&2
	@exit 2
capture-original-help:
	@echo 'Use explicit capture-original.py with canonical provider; old groundwork capture is historical.' >&2
	@exit 2
