PYTHON ?= python3
TOOL = $(PYTHON) wine-tool.py
TC = ../../../toolchains/1.0-nt

.PHONY: all clean check prefix replay audit
all: build/original.exe build/reconstructed.exe build/debug-run.exe

prefix:
	$(PYTHON) wine-tool.py --init

runtime.json: identify-runtime.py binary.py $(TC)/bin/CL.EXE $(TC)/lib/LIBC.LIB $(TC)/lib/SETARGV.OBJ
	$(PYTHON) identify-runtime.py

build/runtime.json: build/cl.exe identify-runtime.py binary.py
	$(PYTHON) identify-runtime.py build/cl.exe $@

build/original.exe build/reconstructed.exe &: build/cl.exe build/runtime.json runtime.json functions.json compare.py binary.py
	$(PYTHON) compare.py --mask-only

check: all audit
	$(PYTHON) compare.py
	$(PYTHON) compare.py --case=test.c

audit: build/runtime.json runtime.json
	$(PYTHON) audit-graph.py

replay: all
	$(PYTHON) compare.py --replay
	$(PYTHON) compare.py --replay --case=test.c

build:
	mkdir -p $@

build/cl.obj: cl.c cl.h | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fobuild/cl.obj cl.c

build/%.obj: %.c cl.h | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fobuild/$*.obj $<

build/cl.exe: build/cl.obj build/stubs.obj build/data.obj build/runtime.obj build/driver.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /map:build/cl.map /out:$@ $^ $(TC)/lib/SETARGV.OBJ

build/debug-run.obj: debug-run.c | build prefix
	$(TOOL) CL.EXE /nologo /c /O1 /G3 /Fobuild/debug-run.obj debug-run.c

build/debug-run.exe: build/debug-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<

clean:
	rm -rf build
