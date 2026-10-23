PYTHON ?= python3
.DEFAULT_GOAL := native
include ../../../tooling/graph-audit/graph-audit.mk
RECON := ../../../lib/recon
NATIVE_CC ?= clang
NATIVE_FLAGS := -m32 -std=gnu89 -fms-extensions -fasm-blocks -O0 -g -fno-pie
NATIVE_CPPFLAGS := -DRECON_CRT_PROFILE_MSVCRT_510 -DRECON_DEFAULT_FMODE=0 -DRECON_NATIVE_TEST -I$(RECON)/include -I$(RECON)/native
SOURCES := $(wildcard src/*.c)
HEADERS := $(wildcard src/*.h $(RECON)/include/*.h $(RECON)/native/*.h)
NATIVE_OBJECTS := $(patsubst src/%.c,build/native/%.o,$(SOURCES))
ORIGINAL_OBJECTS := $(patsubst src/%.c,build/%.obj,$(SOURCES))
SHIMS := $(wildcard $(RECON)/src/*.c)
TOOL := $(PYTHON) scripts/wine-tool.py
.PHONY: native original inspect audit fixtures capture-initial
native: build/native/CVTRES
original: build/CVTRES.EXE
inspect:
	$(PYTHON) scripts/pe_inventory.py
build build/native:
	mkdir -p $@
build/native/%.o: src/%.c $(HEADERS) | build/native
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -finstrument-functions -c $< -o $@
build/native/CVTRES: $(NATIVE_OBJECTS) scripts/native-entry.c $(HEADERS) $(SHIMS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -no-pie $(NATIVE_OBJECTS) scripts/native-entry.c $(SHIMS) -o $@
	$(PYTHON) scripts/build-snapshot.py --kind native --binary $@ --output build/native/inputs.json
build/%.obj: src/%.c $(HEADERS) | build
	$(TOOL) CL.EXE /nologo /c /Od /MD /I$(RECON)/include /Fo$@ $<
build/messages.res: src/resources.rc src/messages.rc src/ce-messages.rc src/diagnostic-messages.rc | build
	$(TOOL) RC.EXE /r /fobuild/messages.res src/resources.rc
build/ce-messages.res: src/ce-messages.rc | build
	$(TOOL) RC.EXE /r /fobuild/ce-messages.res src/ce-messages.rc
build/diagnostic-messages.res: src/diagnostic-messages.rc | build
	$(TOOL) RC.EXE /r /fobuild/diagnostic-messages.res src/diagnostic-messages.rc
build/CVTRES.EXE build/cvtres.map &: $(ORIGINAL_OBJECTS) build/messages.res Makefile
	$(TOOL) LINK.EXE /nologo /subsystem:console /incremental:no /map:build/cvtres.map /out:build/CVTRES.EXE $(ORIGINAL_OBJECTS) build/messages.res
	$(PYTHON) scripts/build-snapshot.py --kind original --binary build/CVTRES.EXE --output build/original-inputs.json
build/capture-host.exe: scripts/capture-host.c | build
	$(TOOL) CL.EXE /nologo /W3 /MT /Od /Fobuild/capture-host.obj /Febuild/capture-host.exe $< /link /incremental:no
	$(PYTHON) scripts/build-snapshot.py --kind capture-host --binary $@ --output build/capture-host-inputs.json
# Original builds and fresh acceptance capture are explicit, outside fixtures.
audit: native original $(GRAPH_AUDIT_BIN)
	$(PYTHON) scripts/audit.py --output build/audit-$(shell date +%s%N)
fixtures:
	$(PYTHON) scripts/fixtures.py
