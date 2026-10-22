PYTHON ?= python3
REPLAY_JOBS ?= 4
TOOL = $(PYTHON) wine-tool.py
TC = ../../../toolchains/1.0-nt

.PHONY: all clean check frontier prefix replay audit hello-frontier hello-check hello-replay coverage
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
	$(PYTHON) replay-frontiers.py --profile minimal --jobs $(REPLAY_JOBS)

hello-frontier: all audit
	$(PYTHON) compare.py --profile hello

hello-check: all audit
	$(PYTHON) compare.py --profile hello --complete

hello-replay: all
	$(PYTHON) replay-frontiers.py --profile hello --jobs $(REPLAY_JOBS)

build:
	mkdir -p $@

build/c2.obj: c2.c c2.h | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fobuild/c2.obj c2.c

build/%.obj: %.c | build prefix
	$(TOOL) CL.EXE /nologo /c /Od /G3 /Fobuild/$*.obj $<

build/c2.exe: build/c2.obj build/stubs.obj build/arguments.obj build/options.obj build/warnings.obj build/initialize.obj build/input.obj build/runtime.obj build/memory.obj build/symbols.obj build/extended.obj build/constant-conversion.obj build/segments.obj build/initializers.obj build/functions.obj build/function-input.obj build/tree-input.obj build/expression-fields.obj build/control-input.obj build/nodes.obj build/leaves.obj build/tree-builder.obj build/canonicalize.obj build/canonical-before.obj build/canonical-after.obj build/tree-types.obj build/function-references.obj build/inline-calls.obj build/inline-candidates.obj build/temporaries.obj build/tree-references.obj build/tree-lowering.obj build/tree-optimizer.obj build/global-tree.obj build/tail-recursion.obj build/optimizer-cache.obj build/algebra.obj build/tree-equality.obj build/tree-cleanup.obj build/work-input.obj build/global-work.obj build/global-registers.obj build/register-transfers.obj build/register-variables.obj build/work-lists.obj build/flow-labels.obj build/flow-edges.obj build/flow-blocks.obj build/flow-order.obj build/flow-dominators.obj build/flow-regions.obj build/flow-rotation.obj build/flow-boundaries.obj build/work-optimizer.obj build/work-alias.obj build/region-motion.obj build/work-expressions.obj build/work-prepare.obj build/parameter-storage.obj build/instruction-selection.obj build/expression-selection.obj build/value-selection.obj build/selection-finishing.obj build/machine-transfers.obj build/instruction-rewrite.obj build/instruction-uses.obj build/function-selection.obj build/register-use.obj build/bitsets.obj build/machine-records.obj build/machine-operations.obj build/machine-operands.obj build/records.obj build/symbol-input.obj build/names.obj build/segment-storage.obj build/backend-flow.obj build/operand-encoding.obj build/address-encoding.obj build/encoding-recipes.obj build/instruction-encoding.obj build/machine-layout.obj build/machine-emission.obj build/final-instructions.obj build/unoptimized.obj build/backend-symbols.obj build/output.obj build/coff-output.obj build/coff-functions.obj build/coff-writer.obj build/coff-finalize.obj build/coff-segments.obj build/listing.obj build/machine-symbols.obj build/initializer-symbols.obj build/symbol-output.obj build/machine-analysis.obj build/machine-simplify.obj build/physical-allocation.obj build/physical-conflicts.obj build/physical-spill.obj build/physical-coalescing.obj build/physical-rewrite.obj build/frame-symbols.obj build/frame-access.obj build/linked-lists.obj build/comparison-branches.obj build/jump-threading.obj build/late-peepholes.obj build/post-allocation.obj build/machine-state.obj build/frame.obj $(TC)/lib/BINMODE.OBJ
	$(TOOL) LINK.EXE /nologo /subsystem:console /include:__fltused /map:build/c2.map /out:$@ $^

build/debug-run.obj: debug-run.c | build prefix
	$(TOOL) CL.EXE /nologo /c /O1 /G3 /Fobuild/debug-run.obj debug-run.c

build/debug-run.exe: build/debug-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<

clean:
	rm -rf build

# Keep header edits local while following transitive project includes.
build/dependencies.mk: dependencies.py $(wildcard *.c) $(wildcard *.h) $(wildcard *.def) | build
	$(PYTHON) dependencies.py

ifneq ($(MAKECMDGOALS),clean)
-include build/dependencies.mk
endif

coverage: runtime.json
	$(PYTHON) coverage.py
