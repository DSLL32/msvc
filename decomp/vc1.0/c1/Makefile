PYTHON ?= python3
TOOL = $(PYTHON) wine-tool.py
TC = ../../../toolchains/1.0-nt

.PHONY: all clean check prefix replay audit
all: build/original.exe build/reconstructed.exe build/debug-run.exe

prefix:
	$(PYTHON) wine-tool.py --init

runtime.json: identify-runtime.py binary.py $(TC)/bin/C13232.EXE $(TC)/lib/LIBC.LIB $(TC)/lib/SETARGV.OBJ
	$(PYTHON) identify-runtime.py

build/runtime.json: build/c1.exe identify-runtime.py binary.py
	$(PYTHON) identify-runtime.py build/c1.exe $@

build/original.exe build/reconstructed.exe &: build/c1.exe build/runtime.json runtime.json functions.json compare.py binary.py
	$(PYTHON) compare.py --mask-only

check: all audit
	$(PYTHON) compare.py

audit: build/runtime.json runtime.json
	$(PYTHON) audit-graph.py

replay: all
	$(PYTHON) compare.py --replay

build:
	mkdir -p $@

build/c1.obj: c1.c c1.h | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fobuild/c1.obj c1.c

build/%.obj: %.c c1.h options.h | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fobuild/$*.obj $<

build/c1.exe: build/c1.obj build/stubs.obj build/diagnostics.obj build/options.obj build/option-data.obj build/packing.obj build/memory.obj build/checksums.obj build/characters.obj build/arguments.obj build/vectors.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /map:build/c1.map /out:$@ $^

build/debug-run.obj: debug-run.c | build prefix
	$(TOOL) CL.EXE /nologo /c /O1 /G3 /Fobuild/debug-run.obj debug-run.c

build/debug-run.exe: build/debug-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<

clean:
	rm -rf build
