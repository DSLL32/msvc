PYTHON ?= python3
TOOL = $(PYTHON) wine-tool.py
TC = ../../../toolchains/1.0-nt
HEADERS = $(wildcard *.h)
OBJECTS = $(addprefix build/,$(addsuffix .obj,link stubs driver files runtime crt-anchors linker repro image symbols external-output section-layout base-relocations arguments options exports library library-options library-stubs import-library import-library-stubs imports ordering parse-switch usage dos-stub link-pass pass-one pass-two machine i386-relocations archive coff-io object comdat comdat-selection arena modules text-buffer diagnostics argument-stubs files-stubs module-stubs driver-stubs machine-stubs pass-stubs object-stubs option-stubs link-pass-stubs pass-two-stubs))

.PHONY: all clean check prefix replay audit
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
	$(PYTHON) audit-callbacks.py
	$(PYTHON) audit-data.py

replay: all audit
	$(PYTHON) compare.py --replay --require-exit

build:
	mkdir -p $@

build/%.obj: %.c $(HEADERS) | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fobuild/$*.obj $<

build/link.exe: $(OBJECTS) Makefile
	$(TOOL) LINK.EXE /nologo /subsystem:console /map:build/link.map /out:$@ $(OBJECTS)

build/debug-run.exe: build/debug-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<

build/linker.obj: image.h arguments.h linker-types.h
build/repro.obj: linker-types.h
build/image.obj: image.h modules.h symbols.h linker-types.h
build/symbols.obj: symbols.h linker-types.h
build/arguments.obj: arguments.h linker-types.h
build/options.obj: options.h pass-one.h image.h modules.h linker-types.h

clean:
	rm -rf build

build/text-buffer.obj: linker-types.h

build/link-pass.obj: link-pass.h object.h options.h comdat.h modules.h image.h dos-stub.h

build/modules.obj: modules.h linker-types.h

build/comdat.obj: comdat.h modules.h linker-types.h

build/arena.obj: linker-types.h

build/pass-one.obj: pass-one.h machine.h comdat.h image.h modules.h linker-types.h

build/files.obj: files.h linker-types.h

build/machine.obj: machine.h image.h modules.h arguments.h comdat.h linker-types.h

build/coff-io.obj: linker-types.h

build/object.obj: object.h pass-one.h image.h modules.h comdat.h linker-types.h

build/usage.obj: linker-types.h

build/diagnostics.obj: linker-types.h

build/argument-stubs.obj: arguments.h linker-types.h
build/files-stubs.obj: files.h linker-types.h
build/parse-switch.obj: options.h image.h modules.h linker-types.h

build/module-stubs.obj: modules.h linker-types.h

build/driver-stubs.obj: image.h modules.h symbols.h linker-types.h

build/machine-stubs.obj: machine.h image.h modules.h linker-types.h

build/pass-stubs.obj: pass-one.h image.h modules.h linker-types.h

build/object-stubs.obj: object.h pass-one.h image.h modules.h comdat.h linker-types.h

build/option-stubs.obj: options.h image.h modules.h linker-types.h

build/link-pass-stubs.obj: link-pass.h object.h options.h comdat.h modules.h image.h
build/dos-stub.obj: dos-stub.h

# Shared behavioral checker is an immutable fixture dependency.
build/gui-check.obj: ../cl/gui-check.c | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fo$@ $<

build/gui-check.exe: build/gui-check.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $< USER32.LIB GDI32.LIB

# The fixture host loads the generated DLL and calls its exported stdcall entry.
build/dll-host.obj: ../fixtures/hello-dll/dll-host.c | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fo$@ $<

build/dll-host.exe: build/dll-host.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $< USER32.LIB
