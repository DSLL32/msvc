PYTHON ?= python3
TC = ../../../toolchains/5.10
RECON = ../../../lib/recon
TOOL = $(PYTHON) scripts/wine-tool.py
NATIVE_CC ?= clang
NATIVE_FLAGS = -m32 -std=gnu89 -fms-extensions -fasm-blocks -fshort-wchar -O0 -g
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -DRECON_LEGACY_JMP_WORDS=16 -DRECON_NATIVE_SETJMP3_COUNT0 -I$(RECON)/include -I$(RECON)/native -Isrc
SOURCES = $(wildcard src/*.c)
HEADERS = $(wildcard src/*.h $(RECON)/include/*.h $(RECON)/native/*.h)
OBJECTS = $(patsubst src/%.c,build/%.obj,$(SOURCES))
NATIVE_OBJECTS = $(patsubst src/%.c,build/native/%.o,$(SOURCES))
NATIVE_SHIMS = $(wildcard $(RECON)/src/*.c)
.PHONY: all native original inspect check-plan fixtures check audit
all: native
include ../../../tooling/graph-audit/graph-audit.mk
inspect:
	$(PYTHON) scripts/pe_inventory.py
check-plan:
	$(PYTHON) scripts/check-plan.py
build build/native:
	mkdir -p $@
build/%.obj: src/%.c $(HEADERS) | build
	$(TOOL) CL.EXE /nologo /c /Od /MD /W3 /I$(RECON)/include /Fo$@ $<
build/RCDLL.DLL build/rcdll.map &: $(OBJECTS) src/rcdll.def build/help.res
	$(TOOL) LINK.EXE /nologo /DLL /entry:recon_dll_entry@12 /def:src/rcdll.def /map:build/rcdll.map /out:build/RCDLL.DLL $(OBJECTS) build/help.res $(TC)/lib/MSVCRT.LIB kernel32.lib user32.lib
	$(PYTHON) scripts/source-snapshot.py original build/original-source.json
original: build/RCDLL.DLL
build/native/%.o: src/%.c $(HEADERS) | build/native
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -finstrument-functions -c $< -o $@
build/native/rcdll-host: $(NATIVE_OBJECTS) scripts/native-host.c $(NATIVE_SHIMS) $(HEADERS)
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_OBJECTS) scripts/native-host.c $(NATIVE_SHIMS) -o $@
	$(PYTHON) scripts/source-snapshot.py native build/native-source.json
native: build/native/rcdll-host build/help-resources.bin
fixtures check: native
	$(PYTHON) scripts/fixtures.py
build/debug-original.obj: scripts/debug-original.c | build
	$(TOOL) CL.EXE /nologo /c /Od /MD /W3 /Fo$@ $<
build/debug-original.exe: build/debug-original.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $< kernel32.lib
graph-audit: $(GRAPH_AUDIT_BIN)
	$(PYTHON) scripts/audit.py
build/message-probe.obj: scripts/message-probe.c | build
	$(TOOL) CL.EXE /nologo /c /Od /MD /W3 /Fo$@ $<
build/message-probe.exe: build/message-probe.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $< kernel32.lib

audit: graph-audit
build/heap-probe.obj: scripts/heap-probe.c | build
	$(TOOL) CL.EXE /nologo /c /Od /MD /W3 /Fo$@ $<
build/heap-probe.exe: build/heap-probe.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $< kernel32.lib

build/help-resources.bin build/help-messages.bin &: src/help.mc cases/help/message-entries.json scripts/prepare-resources.py $(RECON)/resources.py $(TC)/bin/RCDLL.DLL | build
	$(PYTHON) scripts/prepare-resources.py
.PHONY: check-abi
check-abi:
	$(PYTHON) scripts/check-abi.py

build/help.res: src/help.rc build/help-messages.bin
	$(TOOL) RC.EXE /r /fobuild/help.res src/help.rc
