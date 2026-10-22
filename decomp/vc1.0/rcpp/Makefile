PYTHON ?= python3
TOOL = $(PYTHON) scripts/wine-tool.py
TC = ../../../toolchains/1.0-nt
.PHONY: all prefix check replay check-suite replay-suite clean audit classifiers messages coverage interrupt-check option-helper
all: build/rcpp.exe build/debug-run.exe build/interrupt-run.exe
prefix:
	$(TOOL) --init
build:
	mkdir -p $@
build/%.obj: src/%.c $(wildcard src/*.h) | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fobuild/$*.obj $<
build/rcpp.exe: build/startup.obj build/stubs.obj build/runtime.obj build/arguments.obj build/main.obj build/options.obj build/lexer-data.obj build/initialize.obj build/symbols.obj build/file-runtime.obj build/input.obj build/encoding.obj build/line-output.obj build/format.obj build/line-start.obj build/tokens.obj build/directives.obj build/scanning.obj build/directive-names.obj build/lexer.obj build/numbers.obj build/constants.obj build/strings.obj build/includes.obj build/comments.obj build/macro-definition.obj build/condition.obj build/macro-expansion.obj build/macro-identifiers.obj build/macro-arguments.obj build/characters.obj build/messages.obj build/diagnostics.obj build/legacy-ctype.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /entry:rcpp_start /map:build/rcpp.map /out:$@ $^ $(TC)/lib/KERNEL32.LIB
build/debug-run.exe: build/debug-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $< $(TC)/lib/KERNEL32.LIB
audit: all
	$(PYTHON) scripts/prove-library.py
	$(PYTHON) scripts/audit-graph.py
	$(PYTHON) scripts/audit-data.py
check: all audit
	$(PYTHON) scripts/compare.py
replay: all build/option-check.exe
	$(PYTHON) scripts/compare.py --replay
check-suite: all audit
	$(PYTHON) scripts/compare.py --case all
replay-suite: all build/option-check.exe
	$(PYTHON) scripts/compare.py --replay --case all
coverage: all
	$(PYTHON) scripts/coverage.py
clean:
	rm -rf build

build/classifier-check.exe: build/classifier-check.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $< $(TC)/lib/KERNEL32.LIB
classifiers: all build/classifier-check.exe
	$(PYTHON) scripts/check-classifiers.py

build/message-check.exe: build/message-check.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /base:0x02000000 /out:$@ $< $(TC)/lib/KERNEL32.LIB
messages: all build/message-check.exe
	$(PYTHON) scripts/check-messages.py

# Real-console debugger/pipe controller; analysis tooling, excluded from rcpp.exe.
build/interrupt-run.obj: ../fixtures/rcpp-interrupt/capture-interrupt.c | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fo$@ $<
build/interrupt-run.exe: build/interrupt-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $< $(TC)/lib/KERNEL32.LIB

interrupt-check: all
	$(PYTHON) scripts/check-interrupt.py

# Small analysis-only direct case for the otherwise uncalled option helper.
build/option-check.exe: build/option-check.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /base:0x02000000 /out:$@ $< $(TC)/lib/KERNEL32.LIB
option-helper: all build/option-check.exe
	$(PYTHON) scripts/check-option-helper.py
