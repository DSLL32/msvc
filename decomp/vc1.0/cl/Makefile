PYTHON ?= python3
TOOL = $(PYTHON) scripts/wine-tool.py
TC = ../../../toolchains/1.0-nt

.PHONY: all clean check prefix replay audit probe
all: build/original.exe build/reconstructed.exe build/debug-run.exe build/gui-check.exe

prefix:
	$(PYTHON) scripts/wine-tool.py --init

runtime.json: functions.json scripts/identify-runtime.py scripts/binary.py $(TC)/bin/CL.EXE $(TC)/lib/LIBC.LIB $(TC)/lib/SETARGV.OBJ
	$(PYTHON) scripts/identify-runtime.py

build/runtime.json: build/cl.exe functions.json scripts/identify-runtime.py scripts/binary.py
	$(PYTHON) scripts/identify-runtime.py build/cl.exe $@

build/original.exe build/reconstructed.exe &: build/cl.exe build/runtime.json runtime.json functions.json scripts/compare.py scripts/binary.py
	$(PYTHON) scripts/compare.py --mask-only

check: all audit build/paired-run.exe build/pass-proxy.exe build/fail-pass.exe build/wait-pass.exe
	$(PYTHON) scripts/compare.py
	$(PYTHON) scripts/compare.py --case=test.c
	$(PYTHON) scripts/compare.py --case='/O2 /G5 hello.c'
	$(PYTHON) scripts/remaining.py --all

audit: build/runtime.json runtime.json
	$(PYTHON) scripts/audit-graph.py

replay: all build/paired-run.exe build/pass-proxy.exe build/fail-pass.exe build/wait-pass.exe
	$(PYTHON) scripts/compare.py --replay
	$(PYTHON) scripts/compare.py --replay --case=test.c
	$(PYTHON) scripts/compare.py --replay --case='/O2 /G5 hello.c'
	$(PYTHON) scripts/remaining.py --all --replay

build:
	mkdir -p $@

build/cl.obj: src/cl.c src/cl.h | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fobuild/cl.obj src/cl.c

build/%.obj: src/%.c src/cl.h | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fobuild/$*.obj $<

build/cl.exe: build/cl.obj build/stubs.obj build/data.obj build/runtime.obj build/driver.obj build/remaining.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /map:build/cl.map /out:$@ $^ $(TC)/lib/SETARGV.OBJ

build/debug-run.obj: src/debug-run.c | build prefix
	$(TOOL) CL.EXE /nologo /c /O1 /G3 /Fobuild/debug-run.obj src/debug-run.c

build/debug-run.exe: build/debug-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<

build/gui-check.exe: build/gui-check.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $< $(TC)/lib/USER32.LIB $(TC)/lib/GDI32.LIB

build/trace-run.exe: build/trace-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<

build/pass-proxy.exe build/fail-pass.exe build/wait-pass.exe &: build/pass-helper.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:build/pass-proxy.exe $<
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:build/fail-pass.exe $<
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:build/wait-pass.exe $<

probe: all build/trace-run.exe build/pass-proxy.exe build/fail-pass.exe build/wait-pass.exe
	$(PYTHON) scripts/probe-cases.py

clean:
	rm -rf build

build/paired-run.exe: build/paired-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<
