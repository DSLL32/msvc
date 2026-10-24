PYTHON ?= python3
NATIVE_CC ?= clang
RECON = ../../../lib/recon
TC = ../../../toolchains/5.10
TOOL = $(PYTHON) scripts/wine-tool.py
NATIVE_FLAGS = -m32 -std=gnu89 -fms-extensions -fasm-blocks -Wno-ignored-attributes -DRECON_DEFAULT_FMODE=0 -DRECON_CRT_PROFILE_MSVCRT_510 -O0 -g -DRECON_NATIVE_TEST -I$(RECON)/include -I$(RECON)/native -Isrc
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
build/native/%.o: src/%.c $(NATIVE_HEADERS) Makefile | build/native build/native/input-start.json
	$(NATIVE_CC) $(NATIVE_FLAGS) -Dmain=c2_main -DRECON_NATIVE_TRACE -finstrument-functions -c $< -o $@
build/native/c2: build/messages.rmr build/native/stubs.o build/native/startup.o build/native/precision.o build/native/main.o build/native/onexit.o build/native/environment.o build/native/options.o build/native/option-table.o build/native/option-match.o build/native/integer-option.o build/native/il-files.o build/native/il-input.o build/native/diagnostics.o build/native/cleanup.o build/native/symbols.o build/native/nodes.o build/native/constants.o build/native/intrinsics.o build/native/machine-init.o build/native/sections.o build/native/records.o build/native/symbol-input.o build/native/functions.o build/native/assembly-output.o build/native/output-callbacks.o build/native/coff-output.o scripts/native-host.c $(NATIVE_SHIMS) $(RECON)/src/trace.c $(NATIVE_HEADERS) Makefile build/native/input-start.json
	$(NATIVE_CC) $(NATIVE_FLAGS) $(filter %.o,$^) scripts/native-host.c $(NATIVE_SHIMS) $(RECON)/src/trace.c -o $@
	$(PYTHON) scripts/build-provenance.py finish native --binary $@
ORIGINAL_CRT_FLAGS = /MD
# The target compiler intrinsicizes strlen in option action diagnostics.
build/option-match.obj build/diagnostics.obj build/il-files.obj build/symbols.obj build/nodes.obj build/symbol-input.obj build/coff-output.obj: ORIGINAL_EXTRA_FLAGS = /Oi
build/precision.obj: ORIGINAL_CRT_FLAGS = /ML /Zl
build/%.obj: src/%.c $(wildcard src/*.h) Makefile | build build/original/input-start.json
	$(TOOL) CL.EXE /nologo /c /Od /G3 /W3 $(ORIGINAL_CRT_FLAGS) $(ORIGINAL_EXTRA_FLAGS) /I$(RECON)/include /Fo$@ $<
build/messages.rmr: src/messages.rc cases/analysis/encountered-strings.json scripts/prepare-resources.py | build
	$(PYTHON) scripts/prepare-resources.py
build/messages.res: src/messages.rc | build
	$(TOOL) RC.EXE /r /fo build/messages.res src/messages.rc
build/c2.exe: build/messages.res build/stubs.obj build/startup.obj build/precision.obj build/main.obj build/onexit.obj build/environment.obj build/options.obj build/option-table.obj build/option-match.obj build/integer-option.obj build/il-files.obj build/il-input.obj build/diagnostics.obj build/cleanup.obj build/symbols.obj build/nodes.obj build/constants.obj build/intrinsics.obj build/machine-init.obj build/sections.obj build/records.obj build/symbol-input.obj build/functions.obj build/assembly-output.obj build/output-callbacks.obj build/coff-output.obj build/original/input-start.json
	$(TOOL) LINK.EXE /nologo /subsystem:console /entry:mainCRTStartup /map:build/c2.map /out:$@ $(filter %.obj %.res,$^) kernel32.lib user32.lib
	$(PYTHON) scripts/build-provenance.py finish original --binary $@
build/helper/input-start.json: scripts/debug-run.c Makefile $(wildcard $(RECON)/include/*.h) | build
	$(PYTHON) scripts/build-provenance.py begin helper
build/debug-run.obj: scripts/debug-run.c Makefile | build build/helper/input-start.json
	$(TOOL) CL.EXE /nologo /c /Od /G3 /W3 /ML /I$(RECON)/include /Fo$@ $<
build/debug-run.exe: build/debug-run.obj build/helper/input-start.json
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $(filter %.obj,$^)
	$(PYTHON) scripts/build-provenance.py finish helper --binary $@
build/runtime.json build/graph-metadata.json &: build/c2.exe build/c2.map functions.json scripts/build-index.py
	$(PYTHON) scripts/build-index.py
build/c2.map: build/c2.exe
	@test -f $@
build/trial.json: functions.json scripts/state.py | build
	$(PYTHON) -c "import sys; sys.path.insert(0,'scripts'); from state import trial_configuration; from pathlib import Path; Path('$@').write_bytes(trial_configuration(Path('functions.json').read_bytes())[1])"
audit: build/c2.exe build/runtime.json build/graph-metadata.json build/trial.json $(GRAPH_AUDIT_BIN)
	"$(GRAPH_AUDIT_BIN)" --profile cl --repo "$(abspath ../../..)" --project "$(CURDIR)" --original "$(abspath $(TC)/bin/C2.EXE)" --rebuilt "$(CURDIR)/build/c2.exe" --map "$(CURDIR)/build/c2.map" --config "$(CURDIR)/build/trial.json" --runtime "$(CURDIR)/runtime.json" --rebuilt-runtime "$(CURDIR)/build/runtime.json" --metadata "$(CURDIR)/build/graph-metadata.json" --discovery cfg --check-ret --check-imports --check-references --check-accepted-extents --check-ownership --check-extents --report "$(CURDIR)/build/graph-audit.json"
check fixtures: native
	$(PYTHON) scripts/check-native.py
capture-original-help:
	@echo 'Use explicit capture-original.py with canonical provider; old groundwork capture is historical.' >&2
	@exit 2

build/capture/input-start.json: scripts/capture-inputs.c Makefile $(wildcard $(RECON)/include/*.h) | build
	$(PYTHON) scripts/build-provenance.py begin capture
build/capture-inputs.obj: scripts/capture-inputs.c Makefile | build build/capture/input-start.json
	$(TOOL) CL.EXE /nologo /c /Od /G3 /W3 /ML /I$(RECON)/include /Fo$@ $<
build/capture-inputs.exe: build/capture-inputs.obj build/capture/input-start.json
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $(filter %.obj,$^)
	$(PYTHON) scripts/build-provenance.py finish capture --binary $@
