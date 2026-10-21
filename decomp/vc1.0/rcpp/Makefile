PYTHON ?= python3
TOOL = $(PYTHON) wine-tool.py
TC = ../../../toolchains/1.0-nt
.PHONY: all prefix check replay clean audit
all: build/rcpp.exe build/debug-run.exe
prefix:
	$(TOOL) --init
build:
	mkdir -p $@
build/%.obj: %.c rcpp.h preprocessor.h lexer.h | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fobuild/$*.obj $<
build/rcpp.exe: build/startup.obj build/stubs.obj build/runtime.obj build/arguments.obj build/main.obj build/options.obj build/lexer-data.obj build/initialize.obj build/symbols.obj build/file-runtime.obj build/input.obj build/encoding.obj build/line-output.obj build/format.obj build/line-start.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /entry:rcpp_start /map:build/rcpp.map /out:$@ $^ $(TC)/lib/KERNEL32.LIB
build/debug-run.exe: build/debug-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $< $(TC)/lib/KERNEL32.LIB
audit: all
	$(PYTHON) prove-library.py
	$(PYTHON) audit-graph.py
	$(PYTHON) audit-data.py
check: all audit
	$(PYTHON) compare.py
replay: all
	$(PYTHON) compare.py --replay
clean:
	rm -rf build
