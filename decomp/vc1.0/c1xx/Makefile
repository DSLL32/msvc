PYTHON ?= python3
TOOL = $(PYTHON) wine-tool.py
TC = ../../../toolchains/1.0-nt
.PHONY: all prefix check replay audit clean
all: build/original.exe build/reconstructed.exe build/debug-run.exe
prefix:
	$(TOOL) --init
build:
	mkdir -p $@
build/%.obj: %.c c1xx.h options.h characters.h | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fobuild/$*.obj $<
OBJECTS = build/c1xx.obj build/stubs.obj build/diagnostics.obj build/options.obj build/scopes.obj build/packing.obj build/memory.obj build/checksums.obj build/characters.obj build/arguments.obj
build/c1xx.exe: $(OBJECTS) Makefile
	$(TOOL) LINK.EXE /nologo /subsystem:console /map:build/c1xx.map /out:$@ $(OBJECTS)
runtime.json: identify-runtime.py binary.py $(TC)/bin/C1XX3232.EXE $(TC)/lib/LIBC.LIB $(TC)/lib/SETARGV.OBJ
	$(PYTHON) identify-runtime.py
build/runtime.json: build/c1xx.exe identify-runtime.py binary.py
	$(PYTHON) identify-runtime.py build/c1xx.exe $@
build/original.exe build/reconstructed.exe &: build/c1xx.exe build/runtime.json runtime.json functions.json compare.py binary.py
	$(PYTHON) compare.py --mask-only
build/debug-run.exe: build/debug-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<
check: all audit
	$(PYTHON) compare.py
replay: all
	$(PYTHON) compare.py --replay
audit: build/runtime.json runtime.json
	$(PYTHON) audit-graph.py
clean:
	rm -rf build
