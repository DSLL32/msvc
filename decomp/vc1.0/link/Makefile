PYTHON ?= python3
TOOL = $(PYTHON) wine-tool.py
TC = ../../../toolchains/1.0-nt
OBJECTS = $(addprefix build/,$(addsuffix .obj,link stubs driver files runtime crt-anchors linker repro image symbols arguments options parse-switch usage dos-stub assembly-stubs))

.PHONY: all clean check prefix replay audit regenerate
all: build/original.exe build/reconstructed.exe build/debug-run.exe

prefix:
	$(PYTHON) wine-tool.py --init

runtime.json: identify-runtime.py binary.py functions.json $(TC)/bin/LINK.EXE $(TC)/lib/LIBC.LIB
	$(PYTHON) identify-runtime.py

build/runtime.json: build/link.exe identify-runtime.py binary.py functions.json
	$(PYTHON) identify-runtime.py build/link.exe $@

build/original.exe build/reconstructed.exe &: build/link.exe build/runtime.json runtime.json functions.json compare.py binary.py
	$(PYTHON) compare.py --mask-only

check: all audit
	$(PYTHON) compare.py --require-exit

audit: build/runtime.json runtime.json
	$(PYTHON) audit-graph.py
	$(PYTHON) audit-assembly.py
	$(PYTHON) audit-data.py

replay: all audit
	$(PYTHON) compare.py --replay --require-exit

regenerate: runtime.json
	$(PYTHON) lift.py

build:
	mkdir -p $@

build/%.obj: %.c link.h | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fobuild/$*.obj $<

build/link.exe: $(OBJECTS) Makefile
	$(TOOL) LINK.EXE /nologo /subsystem:console /map:build/link.map /out:$@ $(OBJECTS)

build/debug-run.exe: build/debug-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<

build/linker.obj: linker-asm.inc
build/repro.obj: repro-asm.inc
build/image.obj: image-asm.inc
build/symbols.obj: symbols-asm.inc
build/arguments.obj: arguments-asm.inc
build/options.obj: options-asm.inc

clean:
	rm -rf build
