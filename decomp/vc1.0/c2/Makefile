RECON_TOOL_DIR = $(abspath ../../../tooling/recon-tool)
RECON = ../../../lib/recon
NATIVE_CC = clang
NATIVE_CXX = clang++
NATIVE_CPPFLAGS = -DRECON_NATIVE_TRACE -DRECON_NATIVE_TEST -DRECON_DEFAULT_FMODE=0x8000 -DRECON_NATIVE_STARTUP_CONTROLFP_53 -I$(RECON)/include -I$(RECON)/native -Isrc
NATIVE_CFLAGS = -std=gnu89 -fms-extensions -fasm-blocks
BINARY_SOURCES = src/c2.c \
    src/stubs.c \
    src/arguments.c \
    src/options.c \
    src/warnings.c \
    src/diagnostics.c \
    src/initialize.c \
    src/input.c \
    src/memory.c \
    src/symbols.c \
    src/extended.c \
    src/constant-conversion.c \
    src/segments.c \
    src/initializers.c \
    src/functions.c \
    src/exception-scope.c \
    src/function-input.c \
    src/inline-assembly.c \
    src/assembly-operands.c \
    src/tree-input.c \
    src/expression-fields.c \
    src/control-input.c \
    src/nodes.c \
    src/leaves.c \
    src/tree-builder.c \
    src/canonicalize.c \
    src/canonical-before.c \
    src/canonical-after.c \
    src/intrinsics.c \
    src/tree-types.c \
    src/function-references.c \
    src/inline-calls.c \
    src/inline-candidates.c \
    src/temporaries.c \
    src/tree-references.c \
    src/tree-lowering.c \
    src/tree-optimizer.c \
    src/global-tree.c \
    src/tail-recursion.c \
    src/optimizer-cache.c \
    src/integer-fold.c \
    src/algebra.c \
    src/tree-equality.c \
    src/tree-cleanup.c \
    src/work-input.c \
    src/global-work.c \
    src/global-registers.c \
    src/register-transfers.c \
    src/register-variables.c \
    src/work-lists.c \
    src/flow-labels.c \
    src/flow-edges.c \
    src/flow-blocks.c \
    src/flow-order.c \
    src/flow-dominators.c \
    src/flow-regions.c \
    src/flow-rotation.c \
    src/flow-boundaries.c \
    src/work-optimizer.c \
    src/work-alias.c \
    src/region-motion.c \
    src/work-expressions.c \
    src/work-prepare.c \
    src/parameter-storage.c \
    src/instruction-selection.c \
    src/expression-selection.c \
    src/value-selection.c \
    src/intrinsic-selection.c \
    src/selection-finishing.c \
    src/machine-transfers.c \
    src/instruction-rewrite.c \
    src/instruction-uses.c \
    src/function-selection.c \
    src/seh-selection.c \
    src/register-use.c \
    src/bitsets.c \
    src/machine-records.c \
    src/machine-operations.c \
    src/machine-operands.c \
    src/records.c \
    src/symbol-input.c \
    src/names.c \
    src/decorated-names.c \
    src/segment-storage.c \
    src/backend-flow.c \
    src/operand-encoding.c \
    src/address-encoding.c \
    src/encoding-recipes.c \
    src/instruction-encoding.c \
    src/machine-layout.c \
    src/machine-emission.c \
    src/final-instructions.c \
    src/zero-extension.c \
    src/unoptimized.c \
    src/backend-symbols.c \
    src/output.c \
    src/codeview-output.c \
    src/omf-output.c \
    src/coff-output.c \
    src/coff-functions.c \
    src/coff-initializers.c \
    src/coff-writer.c \
    src/coff-finalize.c \
    src/coff-segments.c \
    src/listing.c \
    src/machine-symbols.c \
    src/initializer-symbols.c \
    src/symbol-output.c \
    src/machine-analysis.c \
    src/machine-simplify.c \
    src/physical-allocation.c \
    src/physical-conflicts.c \
    src/physical-spill.c \
    src/physical-coalescing.c \
    src/physical-rewrite.c \
    src/frame-symbols.c \
    src/frame-access.c \
    src/linked-lists.c \
    src/comparison-branches.c \
    src/jump-threading.c \
    src/late-peepholes.c \
    src/scheduler.c \
    src/float-store-split.c \
    src/float-machine.c \
    src/float-peepholes.c \
    src/branch-scheduler.c \
    src/post-allocation.c \
    src/machine-state.c \
    src/frame.c \
    src/frame-output.c
BINARY_SUPPORT_SOURCES = harness/decorated-name-tests.c crt/runtime.c $(wildcard $(RECON)/src/*.c)
ORIGINAL_TOOLCHAIN = 1.0-nt
ORIGINAL_SOURCES = $(BINARY_SOURCES) crt/runtime.c
ORIGINAL_BUILD = build
ORIGINAL_OUTPUT = $(ORIGINAL_BUILD)/c2.exe
ORIGINAL_CPPFLAGS = /Isrc /I$(RECON)/include
ORIGINAL_CFLAGS = /nologo /c /Od /G3
ORIGINAL_LDFLAGS = /nologo /subsystem:console /include:__fltused /map:$(ORIGINAL_BUILD)/c2.map
ORIGINAL_LDLIBS = $(ORIGINAL_TOOLCHAIN_ROOT)/lib/BINMODE.OBJ
ORIGINAL_INCLUDE_DIRS = src $(RECON)/include $(ORIGINAL_TOOLCHAIN_ROOT)/include
RECON_AUDIT_PROFILE = c2
RECON_IMAGE_BASE = 65536
RECON_POLICY = policy-data.json
RECON_AUDIT_FLAGS = -q --discovery cfg --check-ownership --check-extents --check-accepted-extents --ret-checks ret-checks.json
RECON_AUDIT_PREREQUISITES = ret-checks.json
RECON_AUDIT_IDENTIFY_RUNTIME = 1
RECON_RUNTIME_OVERRIDE_OBJECTS = $(ORIGINAL_BUILD)/obj/crt/runtime.c.obj
RECON_RUNTIME_OVERRIDE_MAP = $(ORIGINAL_BUILD)/c2.map
include $(RECON_TOOL_DIR)/recon-tool.mk

# Original VC1 small PODs return EAX:EDX; this object owns every App caller.
$(BINARY_BUILD)/pic/src/decorated-names.c.o: NATIVE_CFLAGS += -freg-struct-return
$(call support-object,harness/decorated-name-tests.c): NATIVE_CFLAGS += -freg-struct-return
