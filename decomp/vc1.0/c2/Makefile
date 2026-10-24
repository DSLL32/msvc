PYTHON ?= python3
TOOL = $(PYTHON) scripts/wine-tool.py
TC = ../../../toolchains/1.0-nt
RECON = ../../../lib/recon

.PHONY: all win32 clean check frontier prefix replay audit hello-frontier hello-check hello-replay coverage
all: native-trace

include ../../../tooling/graph-audit/graph-audit.mk

# Explicit canonical compiler build, outside native fixture execution.
win32: build/c2.exe

prefix:
	$(PYTHON) scripts/wine-tool.py --init

runtime.json: scripts/identify-runtime.py scripts/binary.py scripts/pe_indexes.py $(TC)/bin/C23232.EXE $(TC)/lib/LIBC.LIB
	$(PYTHON) scripts/identify-runtime.py

build/runtime.json: build/c2.exe scripts/identify-runtime.py scripts/binary.py scripts/pe_indexes.py
	$(PYTHON) scripts/identify-runtime.py build/c2.exe $@


check replay: native-trace-check
	$(PYTHON) scripts/check-artifacts.py
	$(PYTHON) scripts/check-omf-artifacts.py

frontier: native-trace
	$(PYTHON) scripts/check-native.py --profile minimal

audit: build/runtime.json runtime.json $(GRAPH_AUDIT_BIN)
	$(GRAPH_AUDIT_BIN) --repo ../../.. --project . --profile c2 --discovery cfg --check-ownership --check-extents --check-accepted-extents --report build/graph-audit.json
	$(PYTHON) scripts/check-frontier-abi.py
	$(PYTHON) scripts/check-codeview-abi.py
	$(PYTHON) scripts/check-listing-output-abi.py
	$(PYTHON) scripts/check-listing-entry-abi.py
	$(PYTHON) scripts/check-coff-entry-abi.py
	$(PYTHON) scripts/check-coff-scalar-abi.py
	$(PYTHON) scripts/check-source-filename-storage.py
	$(PYTHON) scripts/check-codeview-storage.py
	$(PYTHON) scripts/check-debug-symbol-abi.py
	$(PYTHON) scripts/check-codeview-leaf-abi.py
	$(PYTHON) scripts/check-codeview-global-abi.py
	$(PYTHON) scripts/check-codeview-file-abi.py
	$(PYTHON) scripts/check-initializer-release.py
	$(PYTHON) scripts/check-coff-write-lines.py --abi-only
	$(PYTHON) scripts/check-omf-open.py --abi-only
	$(PYTHON) scripts/check-omf-record-cache.py
	$(PYTHON) scripts/check-omf-header.py
	$(PYTHON) scripts/check-omf-begin.py --abi-only
	$(PYTHON) scripts/check-omf-flush-type.py
	$(PYTHON) scripts/check-omf-find.py --abi-only
	$(PYTHON) scripts/check-omf-allocate.py --abi-only
	$(PYTHON) scripts/check-omf-initialize.py --abi-only
	$(PYTHON) scripts/check-omf-string.py
	$(PYTHON) scripts/check-omf-bytes.py --abi-only
	$(PYTHON) scripts/check-omf-flush-wrapper.py
	$(PYTHON) scripts/check-omf-flush.py --abi-only
	$(PYTHON) scripts/check-omf-wire.py --abi-only
	$(PYTHON) scripts/check-omf-directive.py --abi-only
	$(PYTHON) scripts/check-omf-comment.py --abi-only
	$(PYTHON) scripts/check-omf-name-index.py --abi-only
	$(PYTHON) scripts/check-omf-interner.py --abi-only
	$(PYTHON) scripts/check-omf-segment.py --abi-only
	$(PYTHON) scripts/check-omf-alignment.py --abi-only
	$(PYTHON) scripts/check-omf-index.py --abi-only
	$(PYTHON) scripts/check-omf-group.py --abi-only
	$(PYTHON) scripts/check-default-segment-group.py
	$(PYTHON) scripts/check-omf-thread.py --abi-only
	$(PYTHON) scripts/check-omf-select.py --abi-only
	$(PYTHON) scripts/check-omf-data.py --abi-only
	$(PYTHON) scripts/check-omf-reserve.py --abi-only
	$(PYTHON) scripts/check-omf-mark.py --abi-only
	$(PYTHON) scripts/check-omf-public.py --abi-only
	$(PYTHON) scripts/check-omf-external.py --abi-only
	$(PYTHON) scripts/check-omf-relocation.py --abi-only
	$(PYTHON) scripts/check-omf-byte-array.py --abi-only
	$(PYTHON) scripts/check-omf-pad.py --abi-only
	$(PYTHON) scripts/check-omf-datum.py --abi-only
	$(PYTHON) scripts/check-omf-absolute.py --abi-only
	$(PYTHON) scripts/check-omf-fixup.py --abi-only
	$(PYTHON) scripts/check-omf-fixup-allocator.py --abi-only
	$(PYTHON) scripts/check-output-noop-begin.py --abi-only
	$(PYTHON) scripts/check-omf-local-external.py --abi-only
	$(PYTHON) scripts/check-omf-filename.py --abi-only
	$(PYTHON) scripts/check-omf-fixup-writer.py --abi-only
	$(PYTHON) scripts/check-omf-fixup-entry.py --abi-only
	$(PYTHON) scripts/check-omf-symbol-callback.py --abi-only
	$(PYTHON) scripts/check-omf-line-callback.py --abi-only
	$(PYTHON) scripts/check-omf-line-entry.py --abi-only
	$(PYTHON) scripts/check-omf-size.py --abi-only
	$(PYTHON) scripts/check-codeview-block.py --abi-only
	$(PYTHON) scripts/check-omf-relative.py --abi-only
	$(PYTHON) scripts/check-codeview-global-data.py --abi-only
	$(PYTHON) scripts/check-omf-module-end.py --abi-only
	$(PYTHON) scripts/check-omf-modend.py --abi-only

# Explicit Python compatibility/focused proofs remain available; fixtures do not
# invoke a compiler, Cargo or Wine. Data checks retain their individual scope.
.PHONY: audit-python audit-negatives audit-data
audit-python: build/runtime.json runtime.json
	$(PYTHON) scripts/audit-graph.py
audit-negatives: $(GRAPH_AUDIT_BIN)
	$(PYTHON) scripts/check-audit-graph.py
	$(PYTHON) scripts/check-rust-audit.py --binary $(GRAPH_AUDIT_BIN)
	$(PYTHON) scripts/check-stdio-support.py
	$(PYTHON) scripts/check-numeric-support.py
	$(PYTHON) scripts/check-input-support.py
	$(PYTHON) scripts/check-listing-entry-abi.py
	$(PYTHON) scripts/check-coff-entry-abi.py
	$(PYTHON) scripts/check-coff-scalar-abi.py
	$(PYTHON) scripts/check-source-filename-storage.py
	$(PYTHON) scripts/check-codeview-storage.py
	$(PYTHON) scripts/check-debug-symbol-abi.py
	$(PYTHON) scripts/check-codeview-leaf-abi.py
	$(PYTHON) scripts/check-codeview-global-abi.py
	$(PYTHON) scripts/check-codeview-file-abi.py
	$(PYTHON) scripts/check-initializer-release.py
	$(PYTHON) scripts/check-coff-write-lines.py --abi-only
	$(PYTHON) scripts/check-omf-open.py --abi-only
	$(PYTHON) scripts/check-omf-record-cache.py
	$(PYTHON) scripts/check-omf-header.py
	$(PYTHON) scripts/check-omf-begin.py --abi-only
	$(PYTHON) scripts/check-omf-flush-type.py
	$(PYTHON) scripts/check-omf-find.py --abi-only
	$(PYTHON) scripts/check-omf-allocate.py --abi-only
	$(PYTHON) scripts/check-omf-initialize.py --abi-only
	$(PYTHON) scripts/check-omf-string.py
	$(PYTHON) scripts/check-omf-bytes.py --abi-only
	$(PYTHON) scripts/check-omf-flush-wrapper.py
	$(PYTHON) scripts/check-omf-flush.py --abi-only
	$(PYTHON) scripts/check-omf-wire.py --abi-only
	$(PYTHON) scripts/check-omf-directive.py --abi-only
	$(PYTHON) scripts/check-omf-comment.py --abi-only
	$(PYTHON) scripts/check-omf-name-index.py --abi-only
	$(PYTHON) scripts/check-omf-interner.py --abi-only
	$(PYTHON) scripts/check-omf-segment.py --abi-only
	$(PYTHON) scripts/check-omf-alignment.py --abi-only
	$(PYTHON) scripts/check-omf-index.py --abi-only
	$(PYTHON) scripts/check-omf-group.py --abi-only
	$(PYTHON) scripts/check-default-segment-group.py
	$(PYTHON) scripts/check-omf-thread.py --abi-only
	$(PYTHON) scripts/check-omf-select.py --abi-only
	$(PYTHON) scripts/check-omf-data.py --abi-only
	$(PYTHON) scripts/check-omf-reserve.py --abi-only
	$(PYTHON) scripts/check-omf-mark.py --abi-only
	$(PYTHON) scripts/check-omf-public.py --abi-only
	$(PYTHON) scripts/check-omf-external.py --abi-only
	$(PYTHON) scripts/check-omf-relocation.py --abi-only
	$(PYTHON) scripts/check-omf-byte-array.py --abi-only
	$(PYTHON) scripts/check-omf-pad.py --abi-only
	$(PYTHON) scripts/check-omf-datum.py --abi-only
	$(PYTHON) scripts/check-omf-absolute.py --abi-only
	$(PYTHON) scripts/check-omf-fixup.py --abi-only
	$(PYTHON) scripts/check-omf-fixup-allocator.py --abi-only
	$(PYTHON) scripts/check-output-noop-begin.py --abi-only
	$(PYTHON) scripts/check-omf-local-external.py --abi-only
	$(PYTHON) scripts/check-omf-filename.py --abi-only
	$(PYTHON) scripts/check-omf-fixup-writer.py --abi-only
	$(PYTHON) scripts/check-omf-fixup-entry.py --abi-only
	$(PYTHON) scripts/check-omf-symbol-callback.py --abi-only
	$(PYTHON) scripts/check-omf-line-callback.py --abi-only
	$(PYTHON) scripts/check-omf-line-entry.py --abi-only
	$(PYTHON) scripts/check-omf-size.py --abi-only
	$(PYTHON) scripts/check-codeview-block.py --abi-only
	$(PYTHON) scripts/check-omf-relative.py --abi-only
	$(PYTHON) scripts/check-codeview-global-data.py --abi-only
	$(PYTHON) scripts/check-omf-module-end.py --abi-only
	$(PYTHON) scripts/check-omf-modend.py --abi-only
audit-data: build/c2.exe native build/native/check-codeview-string build/native/check-coff-fill build/native/check-listing-directive build/native/check-listing-string build/native/check-assembly-instruction build/native/check-assembly-operand build/native/check-listing-float build/native/check-listing-storage build/native/check-listing-symbol build/native/check-instruction-listing build/native/check-listing-bytes build/native/check-listing-special build/native/check-listing-mnemonic build/native/check-listing-operand build/native/check-listing-prefix build/native/check-register-names build/native/check-instruction-comment build/native/check-listing-comment-prefix build/native/check-listing-type
	$(PYTHON) scripts/check-omf-open.py
	$(PYTHON) scripts/check-coff-write-lines.py
	$(PYTHON) scripts/check-omf-comment.py
	$(PYTHON) scripts/check-omf-bytes.py
	$(PYTHON) scripts/check-omf-name-index.py
	$(PYTHON) scripts/check-omf-wire.py
	$(PYTHON) scripts/check-omf-begin.py
	$(PYTHON) scripts/check-omf-find.py
	$(PYTHON) scripts/check-omf-allocate.py
	$(PYTHON) scripts/check-omf-initialize.py
	$(PYTHON) scripts/check-omf-flush.py
	$(PYTHON) scripts/check-omf-directive.py
	$(PYTHON) scripts/check-omf-interner.py
	$(PYTHON) scripts/check-omf-segment.py
	$(PYTHON) scripts/check-omf-alignment.py
	$(PYTHON) scripts/check-omf-index.py
	$(PYTHON) scripts/check-omf-group.py
	$(PYTHON) scripts/check-default-segment-group.py
	$(PYTHON) scripts/check-omf-thread.py
	$(PYTHON) scripts/check-omf-select.py
	$(PYTHON) scripts/check-omf-data.py
	$(PYTHON) scripts/check-omf-reserve.py
	$(PYTHON) scripts/check-omf-mark.py
	$(PYTHON) scripts/check-omf-public.py
	$(PYTHON) scripts/check-omf-external.py
	$(PYTHON) scripts/check-omf-relocation.py
	$(PYTHON) scripts/check-omf-byte-array.py
	$(PYTHON) scripts/check-omf-pad.py
	$(PYTHON) scripts/check-omf-datum.py
	$(PYTHON) scripts/check-omf-absolute.py
	$(PYTHON) scripts/check-omf-fixup.py
	$(PYTHON) scripts/check-omf-fixup-allocator.py
	$(PYTHON) scripts/check-output-noop-begin.py
	$(PYTHON) scripts/check-omf-local-external.py
	$(PYTHON) scripts/check-omf-filename.py
	$(PYTHON) scripts/check-omf-fixup-writer.py
	$(PYTHON) scripts/check-omf-fixup-entry.py
	$(PYTHON) scripts/check-omf-symbol-callback.py
	$(PYTHON) scripts/check-omf-line-callback.py
	$(PYTHON) scripts/check-omf-line-entry.py
	$(PYTHON) scripts/check-omf-size.py
	$(PYTHON) scripts/check-codeview-block.py
	$(PYTHON) scripts/check-omf-relative.py
	$(PYTHON) scripts/check-codeview-global-data.py
	$(PYTHON) scripts/check-omf-module-end.py
	$(PYTHON) scripts/check-omf-modend.py
	$(PYTHON) scripts/check-float-peephole-data.py
	$(PYTHON) scripts/check-float-stack-effects.py
	$(PYTHON) scripts/check-float-stack-effects.py --scheduler
	$(PYTHON) scripts/check-float-dispatcher-data.py
	$(PYTHON) scripts/check-float-promotion-costs.py
	$(PYTHON) scripts/check-scheduler-stack-operands.py
	$(PYTHON) scripts/check-integer-radix-contract.py
	$(PYTHON) scripts/check-intrinsic-descriptors.py
	$(PYTHON) scripts/check-runtime-intrinsic-data.py
	$(PYTHON) scripts/check-conditional-opcodes.py
	$(PYTHON) scripts/check-scheduler-stack-aliases.py
	$(PYTHON) scripts/check-i386-costs.py
	$(PYTHON) scripts/check-inline-shapes.py
	$(PYTHON) scripts/check-subtraction-overflow.py
	$(PYTHON) scripts/check-debug-segment-storage.py
	$(PYTHON) scripts/check-codeview-input.py
	$(PYTHON) scripts/check-codeview-compile.py
	$(PYTHON) scripts/check-codeview-local-data.py
	$(PYTHON) scripts/check-codeview-typedef.py
	$(PYTHON) scripts/check-codeview-module.py
	$(PYTHON) scripts/check-codeview-frame.py
	$(PYTHON) scripts/check-codeview-label.py
	$(PYTHON) scripts/check-codeview-dispatch.py
	$(PYTHON) scripts/check-codeview-scopes.py
	$(PYTHON) scripts/check-codeview-procedure.py
	$(PYTHON) scripts/check-codeview-string.py
	$(PYTHON) scripts/check-coff-line-boundaries.py
	$(PYTHON) scripts/check-coff-record-line.py
	$(PYTHON) scripts/check-coff-debug-record.py
	$(PYTHON) scripts/check-coff-begin-lines.py
	$(PYTHON) scripts/check-coff-fill.py
	$(PYTHON) scripts/check-coff-fill-contract.py
	$(PYTHON) scripts/check-listing-directive.py
	$(PYTHON) scripts/check-listing-string.py
	$(PYTHON) scripts/check-assembly-instruction.py
	$(PYTHON) scripts/check-assembly-operand.py
	$(PYTHON) scripts/check-listing-float.py
	$(PYTHON) scripts/check-listing-storage.py
	$(PYTHON) scripts/check-listing-symbol.py
	$(PYTHON) scripts/check-instruction-listing.py
	$(PYTHON) scripts/check-listing-bytes.py
	$(PYTHON) scripts/check-listing-special.py
	$(PYTHON) scripts/check-listing-mnemonic.py
	$(PYTHON) scripts/check-listing-operand.py
	$(PYTHON) scripts/check-listing-operand.py --prefix
	$(PYTHON) scripts/check-register-names.py
	$(PYTHON) scripts/check-instruction-comment.py
	$(PYTHON) scripts/check-listing-bytes.py --comment-prefix
	$(PYTHON) scripts/check-listing-directive.py --type


hello-frontier hello-check hello-replay: native-trace
	$(PYTHON) scripts/check-native.py --profile hello



build:
	mkdir -p $@

build/c2.obj: src/c2.c src/c2.h | build prefix
	$(TOOL) CL.EXE /nologo /I$(RECON)/include /c /Od /G3 /Fobuild/c2.obj src/c2.c

build/%.obj: src/%.c | build prefix
	$(TOOL) CL.EXE /nologo /I$(RECON)/include /c /Od /G3 /Fobuild/$*.obj $<

build/c2.exe: build/c2.obj build/stubs.obj build/arguments.obj build/options.obj build/warnings.obj build/initialize.obj build/input.obj build/runtime.obj build/memory.obj build/symbols.obj build/extended.obj build/constant-conversion.obj build/segments.obj build/initializers.obj build/functions.obj build/function-input.obj build/inline-assembly.obj build/assembly-operands.obj build/tree-input.obj build/expression-fields.obj build/control-input.obj build/nodes.obj build/leaves.obj build/tree-builder.obj build/canonicalize.obj build/canonical-before.obj build/canonical-after.obj build/intrinsics.obj build/tree-types.obj build/function-references.obj build/inline-calls.obj build/inline-candidates.obj build/temporaries.obj build/tree-references.obj build/tree-lowering.obj build/tree-optimizer.obj build/global-tree.obj build/tail-recursion.obj build/optimizer-cache.obj build/integer-fold.obj build/algebra.obj build/tree-equality.obj build/tree-cleanup.obj build/work-input.obj build/global-work.obj build/global-registers.obj build/register-transfers.obj build/register-variables.obj build/work-lists.obj build/flow-labels.obj build/flow-edges.obj build/flow-blocks.obj build/flow-order.obj build/flow-dominators.obj build/flow-regions.obj build/flow-rotation.obj build/flow-boundaries.obj build/work-optimizer.obj build/work-alias.obj build/region-motion.obj build/work-expressions.obj build/work-prepare.obj build/parameter-storage.obj build/instruction-selection.obj build/expression-selection.obj build/value-selection.obj build/intrinsic-selection.obj build/selection-finishing.obj build/machine-transfers.obj build/instruction-rewrite.obj build/instruction-uses.obj build/function-selection.obj build/register-use.obj build/bitsets.obj build/machine-records.obj build/machine-operations.obj build/machine-operands.obj build/records.obj build/symbol-input.obj build/names.obj build/segment-storage.obj build/backend-flow.obj build/operand-encoding.obj build/address-encoding.obj build/encoding-recipes.obj build/instruction-encoding.obj build/machine-layout.obj build/machine-emission.obj build/final-instructions.obj build/unoptimized.obj build/backend-symbols.obj build/output.obj build/codeview-output.obj build/omf-output.obj build/coff-output.obj build/coff-functions.obj build/coff-initializers.obj build/coff-writer.obj build/coff-finalize.obj build/coff-segments.obj build/listing.obj build/machine-symbols.obj build/initializer-symbols.obj build/symbol-output.obj build/machine-analysis.obj build/machine-simplify.obj build/physical-allocation.obj build/physical-conflicts.obj build/physical-spill.obj build/physical-coalescing.obj build/physical-rewrite.obj build/frame-symbols.obj build/frame-access.obj build/linked-lists.obj build/comparison-branches.obj build/jump-threading.obj build/late-peepholes.obj build/scheduler.obj build/float-store-split.obj build/float-machine.obj build/float-peepholes.obj build/branch-scheduler.obj build/post-allocation.obj build/machine-state.obj build/frame.obj build/frame-output.obj $(TC)/lib/BINMODE.OBJ
	$(TOOL) LINK.EXE /nologo /subsystem:console /include:__fltused /map:build/c2.map /out:$@ $^

build/debug-run.obj: src/debug-run.c | build prefix
	$(TOOL) CL.EXE /nologo /I$(RECON)/include /c /O1 /G3 /Fobuild/debug-run.obj src/debug-run.c

build/debug-run.exe: build/debug-run.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<

clean:
	rm -rf build

# Keep header edits local while following transitive project includes.
build/dependencies.mk: scripts/dependencies.py $(wildcard src/*.c) $(wildcard src/*.h) $(wildcard src/*.def) | build
	$(PYTHON) scripts/dependencies.py

ifneq ($(MAKECMDGOALS),clean)
-include build/dependencies.mk
endif

coverage: runtime.json
	$(PYTHON) scripts/coverage.py

# Native regression builds have no Wine or original-toolchain prerequisites.
NATIVE_CC ?= clang
NATIVE_FLAGS = -m32 -std=gnu89 -fms-extensions -fasm-blocks -O0 -g
NATIVE_CPPFLAGS = -DRECON_NATIVE_TEST -DRECON_DEFAULT_FMODE=0x8000 -DRECON_NATIVE_STARTUP_CONTROLFP_53 -I$(RECON)/include -I$(RECON)/native
NATIVE_HEADERS = $(wildcard src/*.h src/*.def $(RECON)/include/*.h $(RECON)/native/*.h)
NATIVE_SHIMS = $(filter-out $(RECON)/src/trace.c,$(wildcard $(RECON)/src/*.c))
NATIVE_OBJECTS = build/native/c2.o build/native/stubs.o build/native/arguments.o build/native/options.o build/native/warnings.o build/native/initialize.o build/native/input.o build/native/runtime.o build/native/memory.o build/native/symbols.o build/native/extended.o build/native/constant-conversion.o build/native/segments.o build/native/initializers.o build/native/functions.o build/native/function-input.o build/native/inline-assembly.o build/native/assembly-operands.o build/native/tree-input.o build/native/expression-fields.o build/native/control-input.o build/native/nodes.o build/native/leaves.o build/native/tree-builder.o build/native/canonicalize.o build/native/canonical-before.o build/native/canonical-after.o build/native/intrinsics.o build/native/tree-types.o build/native/function-references.o build/native/inline-calls.o build/native/inline-candidates.o build/native/temporaries.o build/native/tree-references.o build/native/tree-lowering.o build/native/tree-optimizer.o build/native/global-tree.o build/native/tail-recursion.o build/native/optimizer-cache.o build/native/integer-fold.o build/native/algebra.o build/native/tree-equality.o build/native/tree-cleanup.o build/native/work-input.o build/native/global-work.o build/native/global-registers.o build/native/register-transfers.o build/native/register-variables.o build/native/work-lists.o build/native/flow-labels.o build/native/flow-edges.o build/native/flow-blocks.o build/native/flow-order.o build/native/flow-dominators.o build/native/flow-regions.o build/native/flow-rotation.o build/native/flow-boundaries.o build/native/work-optimizer.o build/native/work-alias.o build/native/region-motion.o build/native/work-expressions.o build/native/work-prepare.o build/native/parameter-storage.o build/native/instruction-selection.o build/native/expression-selection.o build/native/value-selection.o build/native/intrinsic-selection.o build/native/selection-finishing.o build/native/machine-transfers.o build/native/instruction-rewrite.o build/native/instruction-uses.o build/native/function-selection.o build/native/register-use.o build/native/bitsets.o build/native/machine-records.o build/native/machine-operations.o build/native/machine-operands.o build/native/records.o build/native/symbol-input.o build/native/names.o build/native/segment-storage.o build/native/backend-flow.o build/native/operand-encoding.o build/native/address-encoding.o build/native/encoding-recipes.o build/native/instruction-encoding.o build/native/machine-layout.o build/native/machine-emission.o build/native/final-instructions.o build/native/unoptimized.o build/native/backend-symbols.o build/native/output.o build/native/codeview-output.o build/native/omf-output.o build/native/coff-output.o build/native/coff-functions.o build/native/coff-initializers.o build/native/coff-writer.o build/native/coff-finalize.o build/native/coff-segments.o build/native/listing.o build/native/machine-symbols.o build/native/initializer-symbols.o build/native/symbol-output.o build/native/machine-analysis.o build/native/machine-simplify.o build/native/physical-allocation.o build/native/physical-conflicts.o build/native/physical-spill.o build/native/physical-coalescing.o build/native/physical-rewrite.o build/native/frame-symbols.o build/native/frame-access.o build/native/linked-lists.o build/native/comparison-branches.o build/native/jump-threading.o build/native/late-peepholes.o build/native/scheduler.o build/native/float-store-split.o build/native/float-machine.o build/native/float-peepholes.o build/native/branch-scheduler.o build/native/post-allocation.o build/native/machine-state.o build/native/frame.o build/native/frame-output.o
NATIVE_TRACE_OBJECTS = $(patsubst build/native/%,build/native-trace/%,$(NATIVE_OBJECTS))
.PHONY: native native-trace native-check native-trace-check
native: build/native/C23232.EXE
native-trace: build/native-trace/C23232.EXE
build/native build/native-trace:
	mkdir -p $@
build/native/%.o: src/%.c $(NATIVE_HEADERS) | build/native
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -c $< -o $@
build/native-trace/%.o: src/%.c $(NATIVE_HEADERS) | build/native-trace
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -DRECON_NATIVE_TRACE -finstrument-functions -c $< -o $@
build/native-trace/runtime.o: src/runtime.c $(NATIVE_HEADERS) | build/native-trace
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -c $< -o $@
build/native/C23232.EXE: $(NATIVE_OBJECTS) $(NATIVE_SHIMS) $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_OBJECTS) $(NATIVE_SHIMS) -o $@
build/native-trace/C23232.EXE: $(NATIVE_TRACE_OBJECTS) $(NATIVE_SHIMS) $(RECON)/src/trace.c $(NATIVE_HEADERS) Makefile
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) $(NATIVE_TRACE_OBJECTS) $(NATIVE_SHIMS) $(RECON)/src/trace.c -o $@

native-check: native
	$(PYTHON) scripts/check-native.py --no-trace
native-trace-check: native-trace
	$(PYTHON) scripts/check-native.py --reject-checks

# Empty compatibility headers must also trigger canonical recompilation.
$(patsubst build/native/%.o,build/%.obj,$(NATIVE_OBJECTS)): $(wildcard $(RECON)/include/*.h)

# Explicit original-only CL transport capture helper; never used by fixtures.
build/capture-cl.obj: src/capture-cl.c | build prefix
	$(TOOL) CL.EXE /nologo /I$(RECON)/include /c /Od /G3 /Fobuild/capture-cl.obj $<
build/capture-cl.exe: build/capture-cl.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<

# Explicit original-only CRT boundary observer, excluded from native fixtures.
build/reference-strpbrk-observer.exe: build/reference-strpbrk-observer.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $^

build/reference-freopen-observer.exe: build/reference-freopen-observer.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $^

build/reference-stdhandle-observer.exe: build/reference-stdhandle-observer.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $^

build/reference-debug-segment-observer.exe: build/reference-debug-segment-observer.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<

build/reference-printf-observer.exe: build/reference-printf-observer.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<

build/reference-stdio-observer.exe: build/reference-stdio-observer.obj
	$(TOOL) LINK.EXE /nologo /subsystem:console /out:$@ $<

# Isolated native C helper test: discard unrelated output-writer sections.
build/native/check-codeview-string: src/check-codeview-string.c src/codeview-output.c $(NATIVE_HEADERS)
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -ffunction-sections -fdata-sections src/check-codeview-string.c src/codeview-output.c -Wl,--gc-sections -o $@

build/native/check-coff-fill: src/check-coff-fill.c src/coff-writer.c $(NATIVE_HEADERS)
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -ffunction-sections -fdata-sections src/check-coff-fill.c src/coff-writer.c -Wl,--gc-sections -o $@

build/native/check-listing-directive: src/check-listing-directive.c src/listing.c $(NATIVE_HEADERS)
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -ffunction-sections -fdata-sections src/check-listing-directive.c src/listing.c -Wl,--gc-sections -o $@

build/native/check-listing-string: src/check-listing-string.c src/listing.c $(NATIVE_HEADERS)
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -ffunction-sections -fdata-sections src/check-listing-string.c src/listing.c -Wl,--gc-sections -o $@

build/native/check-assembly-instruction: src/check-assembly-instruction.c src/inline-assembly.c src/machine-records.c src/machine-operands.c src/machine-symbols.c $(NATIVE_HEADERS)
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -ffunction-sections -fdata-sections src/check-assembly-instruction.c src/inline-assembly.c src/machine-records.c src/machine-operands.c src/machine-symbols.c -Wl,--gc-sections -o $@

build/native/check-assembly-operand: src/check-assembly-operand.c src/assembly-operands.c src/machine-operands.c src/algebra.c $(NATIVE_HEADERS)
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -ffunction-sections -fdata-sections src/check-assembly-operand.c src/assembly-operands.c src/machine-operands.c src/algebra.c -Wl,--gc-sections -o $@

build/native/check-listing-float: src/check-listing-float.c src/listing.c $(NATIVE_HEADERS) $(NATIVE_SHIMS)
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -ffunction-sections -fdata-sections src/check-listing-float.c src/listing.c $(NATIVE_SHIMS) -Wl,--gc-sections -o $@

build/native/check-listing-storage: src/check-listing-storage.c src/listing.c $(NATIVE_HEADERS)
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -ffunction-sections -fdata-sections src/check-listing-storage.c src/listing.c -Wl,--gc-sections -o $@

build/native/check-listing-symbol: src/check-listing-symbol.c src/listing.c $(NATIVE_HEADERS)
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -ffunction-sections -fdata-sections src/check-listing-symbol.c src/listing.c -Wl,--gc-sections -o $@

build/native/listing-driver-body.c: src/listing.c scripts/extract-listing-driver.py | build/native
	$(PYTHON) scripts/extract-listing-driver.py $@

build/native/check-instruction-listing: src/check-instruction-listing.c build/native/listing-driver-body.c src/register-use.c $(NATIVE_HEADERS)
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -ffunction-sections -fdata-sections src/check-instruction-listing.c build/native/listing-driver-body.c src/register-use.c -Wl,--gc-sections -o $@

build/native/listing-bytes-body.c: src/listing.c scripts/extract-listing-driver.py | build/native
	$(PYTHON) scripts/extract-listing-driver.py $@ bytes

build/native/check-listing-bytes: src/check-listing-bytes.c build/native/listing-bytes-body.c $(NATIVE_HEADERS)
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) src/check-listing-bytes.c build/native/listing-bytes-body.c -o $@

build/native/listing-special-body.c: src/listing.c scripts/extract-listing-driver.py | build/native
	$(PYTHON) scripts/extract-listing-driver.py $@ special

build/native/check-listing-special: src/check-listing-special.c build/native/listing-special-body.c src/register-use.c src/machine-symbols.c $(NATIVE_HEADERS)
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -ffunction-sections -fdata-sections src/check-listing-special.c build/native/listing-special-body.c src/register-use.c src/machine-symbols.c -Wl,--gc-sections -o $@

build/native/listing-mnemonic-body.c: src/listing.c scripts/extract-listing-driver.py | build/native
	$(PYTHON) scripts/extract-listing-driver.py $@ mnemonic

build/native/check-listing-mnemonic: src/check-listing-mnemonic.c build/native/listing-mnemonic-body.c src/listing-mnemonics.def src/register-use.c src/machine-symbols.c $(NATIVE_HEADERS)
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -Isrc -ffunction-sections -fdata-sections src/check-listing-mnemonic.c build/native/listing-mnemonic-body.c src/register-use.c src/machine-symbols.c -Wl,--gc-sections -o $@

build/native/listing-operand-body.c: src/listing.c scripts/extract-listing-driver.py | build/native
	$(PYTHON) scripts/extract-listing-driver.py $@ operand

build/native/check-listing-operand: src/check-listing-operand.c build/native/listing-operand-body.c $(NATIVE_HEADERS)
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) src/check-listing-operand.c build/native/listing-operand-body.c -o $@

build/native/listing-prefix-body.c: src/listing.c scripts/extract-listing-driver.py | build/native
	$(PYTHON) scripts/extract-listing-driver.py $@ prefix

build/native/check-listing-prefix: src/check-listing-operand.c build/native/listing-prefix-body.c $(NATIVE_HEADERS)
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -DCHECK_PREFIX src/check-listing-operand.c build/native/listing-prefix-body.c -o $@

build/native/check-register-names: src/check-register-names.c src/listing.c $(NATIVE_HEADERS)
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -ffunction-sections -fdata-sections src/check-register-names.c src/listing.c -Wl,--gc-sections -o $@

build/native/listing-comment-body.c: src/listing.c scripts/extract-listing-driver.py | build/native
	$(PYTHON) scripts/extract-listing-driver.py $@ comment

build/native/check-instruction-comment: src/check-instruction-comment.c build/native/listing-comment-body.c src/machine-symbols.c $(NATIVE_HEADERS)
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -ffunction-sections -fdata-sections src/check-instruction-comment.c build/native/listing-comment-body.c src/machine-symbols.c -Wl,--gc-sections -o $@

build/native/listing-comment-prefix-body.c: src/listing.c scripts/extract-listing-driver.py | build/native
	$(PYTHON) scripts/extract-listing-driver.py $@ comment_prefix

build/native/check-listing-comment-prefix: src/check-listing-bytes.c build/native/listing-comment-prefix-body.c $(NATIVE_HEADERS)
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -DCHECK_COMMENT_PREFIX src/check-listing-bytes.c build/native/listing-comment-prefix-body.c -o $@

build/native/check-listing-type: src/check-listing-directive.c src/listing.c $(NATIVE_HEADERS)
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -DCHECK_LISTING_TYPE -ffunction-sections -fdata-sections src/check-listing-directive.c src/listing.c -Wl,--gc-sections -o $@

build/native/listing-displacement-body.c: src/listing.c scripts/extract-listing-driver.py | build/native
	$(PYTHON) scripts/extract-listing-driver.py $@ displacement

build/native/check-listing-displacement: src/check-listing-bytes.c build/native/listing-displacement-body.c $(NATIVE_HEADERS)
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -DCHECK_DISPLACEMENT src/check-listing-bytes.c build/native/listing-displacement-body.c -o $@

build/native/check-coff-filename: src/check-coff-filename.c src/coff-functions.c $(NATIVE_HEADERS)
	$(NATIVE_CC) $(NATIVE_CPPFLAGS) $(NATIVE_FLAGS) -ffunction-sections -fdata-sections src/check-coff-filename.c src/coff-functions.c -Wl,--gc-sections -o $@
