PYTHON ?= python3
TOOL = $(PYTHON) wine-tool.py
TC = ../../../toolchains/1.0-nt
.PHONY: all prefix check replay check-suite replay-suite clean audit classifiers messages coverage
all: build/rcpp.exe build/debug-run.exe
prefix:
	$(TOOL) --init
build:
	mkdir -p $@
build/%.obj: %.c rcpp.h preprocessor.h lexer.h tokens.h directives.h numbers.h condition.h macros.h legacy-ctype.h | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fobuild/$*.obj $<
build/rcpp.exe: build/startup.obj build/stubs.obj build/runtime.obj build/arguments.obj build/main.obj build/options.obj build/lexer-data.obj build/initialize.obj build/symbols.obj build/file-runtime.obj build/input.obj build/encoding.obj build/line-output.obj build/format.obj build/line-start.obj build/tokens.obj build/directives.obj build/scanning.obj build/directive-names.obj build/lexer.obj build/numbers.obj build/constants.obj build/strings.obj build/includes.obj build/comments.obj build/macro-definition.obj build/condition.obj build/macro-expansion.obj build/macro-identifiers.obj build/macro-arguments.obj build/characters.obj build/messages.obj build/diagnostics.obj build/legacy-ctype.obj
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
check-suite: all audit
	$(PYTHON) compare.py --case all
replay-suite: all
	$(PYTHON) compare.py --replay --case all
coverage: all
	$(PYTHON) coverage.py
clean:
	rm -rf build

build/classifier-check.exe: build/classifier-check.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $< $(TC)/lib/KERNEL32.LIB
classifiers: all build/classifier-check.exe
	$(PYTHON) check-classifiers.py

build/message-check.exe: build/message-check.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /base:0x02000000 /out:$@ $< $(TC)/lib/KERNEL32.LIB
messages: all build/message-check.exe
	$(PYTHON) check-messages.py
