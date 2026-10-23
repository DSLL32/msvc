PYTHON ?= python3
TOOL = $(PYTHON) scripts/wine-tool.py
TC = ../../../toolchains/1.0-nt
RECON = ../../../lib/recon
.PHONY: all prefix check replay check-suite replay-suite clean audit classifiers messages coverage interrupt-check option-helper
all: native-trace
include ../../../tooling/graph-audit/graph-audit.mk
win32: build/rcpp.exe build/debug-run.exe build/interrupt-run.exe
prefix:
	$(TOOL) --init
build:
	mkdir -p $@
build/%.obj: src/%.c $(wildcard src/*.h) $(RECON)/include/recon.h | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /I$(RECON)/include /Fobuild/$*.obj $<
OBJECTS = build/startup.obj build/stubs.obj build/runtime.obj build/arguments.obj build/main.obj build/options.obj build/lexer-data.obj build/initialize.obj build/symbols.obj build/file-runtime.obj build/input.obj build/encoding.obj build/line-output.obj build/format.obj build/line-start.obj build/tokens.obj build/directives.obj build/scanning.obj build/directive-names.obj build/lexer.obj build/numbers.obj build/constants.obj build/strings.obj build/includes.obj build/comments.obj build/macro-definition.obj build/condition.obj build/macro-expansion.obj build/macro-identifiers.obj build/macro-arguments.obj build/characters.obj build/messages.obj build/diagnostics.obj build/legacy-ctype.obj
build/rcpp.exe: $(OBJECTS)
	$(TOOL) LINK.EXE /nologo /subsystem:console /entry:rcpp_start /map:build/rcpp.map /out:$@ $^ $(TC)/lib/KERNEL32.LIB
build/debug-run.exe: build/debug-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $< $(TC)/lib/KERNEL32.LIB
audit: build/rcpp.exe
	$(MAKE) audit-static

# Existing canonical PE/MAP artifacts only; no Wine, compiler or recapture.
.PHONY: graph-audit audit-static
graph-audit: $(GRAPH_AUDIT_BIN) | build
	$(GRAPH_AUDIT_BIN) --repo ../../.. --project . --profile rcpp --discovery cfg --check-ret --check-references --check-ownership --check-extents --check-accepted-extents --report build/graph.json
	$(PYTHON) scripts/audit-graph.py
audit-static: graph-audit
	$(PYTHON) scripts/prove-library.py
	$(PYTHON) scripts/audit-data.py
# Native fixtures never depend on Wine, reference builders or PE execution.
check replay check-suite replay-suite: native-trace-check

.PHONY: capture-original
capture-original: build/debug-run.exe build/interrupt-run.exe
	$(PYTHON) scripts/reference.py --case all
coverage: build/rcpp.exe
	$(PYTHON) scripts/coverage.py
clean:
	rm -rf build

classifiers: build/native-trace/classifier-check
	$(PYTHON) scripts/check-classifiers.py

messages: build/native-trace/message-check
	$(PYTHON) scripts/check-messages.py

# Real-console debugger/pipe controller; analysis tooling, excluded from rcpp.exe.
build/interrupt-run.obj: ../fixtures/rcpp-interrupt/capture-interrupt.c | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fo$@ $<
build/interrupt-run.exe: build/interrupt-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $< $(TC)/lib/KERNEL32.LIB

interrupt-check: native-trace
	$(PYTHON) scripts/check-native.py --trace --case console-interrupt

# Small analysis-only direct case for the otherwise uncalled option helper.
option-helper: native-trace build/native-trace/option-check
	$(PYTHON) scripts/check-native.py --trace --case option-helper

# Native fixtures compile shared compatibility C directly at final link.
NATIVE_CC ?= clang
NATIVE_FLAGS = -m32 -std=gnu89 -fms-extensions -fasm-blocks -O0 -g
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -I$(RECON)/include -I$(RECON)/native
# Identified embedded CRT startup/argv/heap implementations are substituted at
# the explicitly authorized CRT boundary; native application bodies are retained.
NATIVE_OBJECTS = $(patsubst build/%.obj,build/native/%.o,$(filter-out build/startup.obj build/stubs.obj build/runtime.obj build/arguments.obj,$(OBJECTS)))
NATIVE_TRACE_OBJECTS = $(patsubst build/native/%,build/native-trace/%,$(NATIVE_OBJECTS))
NATIVE_HEADERS = $(wildcard src/*.h $(RECON)/include/*.h $(RECON)/native/*.h)
NATIVE_SHIMS = $(filter-out $(RECON)/src/trace.c,$(wildcard $(RECON)/src/*.c))

.PHONY: native native-trace native-check native-trace-check
native: build/native/RCPP.EXE
native-trace: build/native-trace/RCPP.EXE
build/native build/native-trace:
	mkdir -p $@
build/native/%.o: src/%.c $(NATIVE_HEADERS) | build/native
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -c $< -o $@
build/native-trace/%.o: src/%.c $(NATIVE_HEADERS) | build/native-trace
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -finstrument-functions -c $< -o $@
build/native-trace/file-runtime.o: src/file-runtime.c $(NATIVE_HEADERS) | build/native-trace
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -c $< -o $@
build/native/RCPP.EXE: $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -DRECON_NATIVE_ENTRY=rc_main $(NATIVE_OBJECTS) $(NATIVE_SHIMS) -o $@
build/native-trace/RCPP.EXE: $(NATIVE_TRACE_OBJECTS) $(NATIVE_SHIMS) $(RECON)/src/trace.c $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -DRECON_NATIVE_ENTRY=rc_main $(NATIVE_TRACE_OBJECTS) $(NATIVE_SHIMS) $(RECON)/src/trace.c -o $@
native-check: native build/native-trace/option-check
	$(PYTHON) scripts/check-native.py
native-trace-check: native-trace build/native-trace/option-check build/native-trace/classifier-check build/native-trace/message-check
	$(PYTHON) scripts/check-native.py --trace
	$(PYTHON) scripts/check-classifiers.py
	$(PYTHON) scripts/check-messages.py

# Existing direct helper ABI case, linked as ELF rather than loading a PE.
build/native-tests:
	mkdir -p $@
build/native-tests/option-check.o: src/native-option-check.c $(NATIVE_HEADERS) | build/native-tests
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -c $< -o $@
build/native-trace/option-check: build/native-tests/option-check.o $(NATIVE_TRACE_OBJECTS) $(NATIVE_SHIMS) $(RECON)/src/trace.c $(NATIVE_HEADERS)
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) build/native-tests/option-check.o $(NATIVE_TRACE_OBJECTS) $(NATIVE_SHIMS) $(RECON)/src/trace.c -o $@

build/native-tests/%-check.o: src/native-%-check.c $(NATIVE_HEADERS) | build/native-tests
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -c $< -o $@
build/native-trace/classifier-check: build/native-tests/classifier-check.o $(NATIVE_TRACE_OBJECTS) $(NATIVE_SHIMS) $(RECON)/src/trace.c $(NATIVE_HEADERS)
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $< $(NATIVE_TRACE_OBJECTS) $(NATIVE_SHIMS) $(RECON)/src/trace.c -o $@
build/native-trace/message-check: build/native-tests/message-check.o $(NATIVE_TRACE_OBJECTS) $(NATIVE_SHIMS) $(RECON)/src/trace.c $(NATIVE_HEADERS)
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $< $(NATIVE_TRACE_OBJECTS) $(NATIVE_SHIMS) $(RECON)/src/trace.c -o $@

.PHONY: native-clean native-reject-check
native-clean:
	rm -rf build/native build/native-trace build/native-tests
native-reject-check: native-trace build/native-trace/option-check $(RECON)/build/process-target
	$(PYTHON) scripts/check-native.py --trace --reject-tests

.SECONDARY: build/native-tests/classifier-check.o build/native-tests/message-check.o

$(RECON)/build/process-target:
	$(MAKE) -C $(RECON) -j4 build/process-target
