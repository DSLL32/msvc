PYTHON ?= python3
TOOL = $(PYTHON) wine-tool.py
TC = ../../../toolchains/1.0-nt

.PHONY: all clean check frontier prefix replay audit
all: build/original.exe build/reconstructed.exe build/debug-run.exe

prefix:
	$(PYTHON) wine-tool.py --init

runtime.json: identify-runtime.py binary.py $(TC)/bin/C23232.EXE $(TC)/lib/LIBC.LIB
	$(PYTHON) identify-runtime.py

build/runtime.json: build/c2.exe identify-runtime.py binary.py
	$(PYTHON) identify-runtime.py build/c2.exe $@

build/original.exe build/reconstructed.exe &: build/c2.exe build/runtime.json runtime.json functions.json compare.py binary.py
	$(PYTHON) compare.py --mask-only

check: all audit
	$(PYTHON) compare.py --complete

frontier: all audit
	$(PYTHON) compare.py

audit: build/runtime.json runtime.json
	$(PYTHON) audit-graph.py

replay: all
	$(PYTHON) compare.py --replay

build:
	mkdir -p $@

build/c2.obj: c2.c c2.h | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fobuild/c2.obj c2.c

build/%.obj: %.c c2.h options.h input.h backend.h extended.h segments.h symbols.h initializers.h functions.h records.h | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fobuild/$*.obj $<

build/c2.exe: build/c2.obj build/stubs.obj build/arguments.obj build/options.obj build/initialize.obj build/input.obj build/runtime.obj build/memory.obj build/symbols.obj build/extended.obj build/segments.obj build/initializers.obj build/functions.obj build/records.obj build/symbol-input.obj build/names.obj build/segment-storage.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /include:__fltused /map:build/c2.map /out:$@ $^

build/debug-run.obj: debug-run.c | build prefix
	$(TOOL) CL.EXE /nologo /c /O1 /G3 /Fobuild/debug-run.obj debug-run.c

build/debug-run.exe: build/debug-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<

clean:
	rm -rf build
