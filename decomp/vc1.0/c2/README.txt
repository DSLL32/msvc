C23232.EXE 1.0-nt reconstruction (project vc1.0)

Status: captured minimal-C backend case complete. Both masked executions and
the unmodified original exit zero with the expected 286-byte object, matching
all bytes except the COFF timestamp. Binary reconstruction continues with the
queued optimized Win32 hello case. The minimal checkpoint covers 275 source
functions; subsequent hello work is tracked in cases/hello.json. Optional paths
retain named traps.

Target: toolchains/1.0-nt/bin/C23232.EXE
SHA256: 0a64eab35cfb4bfb8af452653d21707947ada33952a82a5e712c275ab7712d26
Original image base: 00010000; entry point: 00073470.

The starting case is ../fixtures/minimal-c/manifest.json, phases.c2. Each run
gets an isolated working directory containing the four captured frontend files
irex, irsy, irin and irgl. No test.c file is needed. The debug launcher passes
exactly one environment variable, MSC_CMD_FLAGS, with the captured options and
the portable relative intermediate prefix "ir". The child has no extra argv
arguments. The original target hash and fixture input hashes are checked.

    make -j2 all       build with the original toolchain using Wine
    make frontier     compare the current case and audit function graph
    make replay       rerun every recorded accepted trap frontier
    make check        require complete exit 0 and exact object output

make check validates successful completion of the captured minimal-C case.
For the final output comparison, only the four-byte COFF TimeDateStamp field
at object offset 4 is ignored. Every other byte must match the captured
286-byte test.obj. Both output streams must be empty and exit status must be 0.

wine-tool.py owns ~/.ai-tmp/vc1.0-c2/wine. Toolchains and fixtures are read-only.
The Makefile supports parallel builds. Generated files are under build/.

Both images have all code-section bytes replaced with INT3. compare.py restores
only accepted function ranges, including function-owned .text tables. CRT code
is identified from original LIBC.LIB using relocation-masked byte matches plus
relative-call target checks. An individual-function fallback handles the older
ieee87 object; private function symbols bound its extents. Short candidates
with unresolved call relocations are rejected. The original linked CRT group
begins at freopen (00071aa0); generic RET/JMP shapes in backend code do not
establish CRT identity. Distinct compiler functions have named trapping stubs.

The older _sopen, _filbuf and _freebuf CRT variants match those already reconstructed for
CL. They are explicit source in runtime.c, with their C2 extents recorded.
Floating-point CRT startup is retained by linking __fltused. The target links
BINMODE.OBJ, giving _fmode = 0x8000; this matters because its COFF adapter opens
object files with "w". The Makefile selects the same original runtime object,
and compare.py checks this named CRT default in both images. The backend's own
extended-float routines use explicit x87/ESI private ABIs in extended.c.

Data definitions are introduced with the functions that encounter them:
options.c contains named switch destinations, typed switch/suboption tables
and option parsing; input.c contains intermediate input; memory.c owns eight
arena descriptors; symbols.c owns descriptor bookkeeping. No original data
section snapshots are linked into this reconstruction.

The graph audit checks direct calls, indirect-call presence, and tail calls
for accepted source functions. It follows reachable code, switch-table targets,
and MSVC SEH filter/handler entries, rather than decoding table data as code.
functions.json records accepted frontier order;
runtime.json records library function identities; unaccepted stub extents are
provisional and are refined when reached. Completed functions are reconstructed
in full, including branches not exercised by this case.

Queued case: ../fixtures/hello-win32/manifest.json, phases.c2, captured from
original CL /O2 /G5 hello.c. It follows the current minimal-C case. See
cases/queue.json. It has four captured frontend files and no source-file input;
the exact captured backend options include -Gs4096 -Gy -G5. The expected object
has 2289 bytes, with only its COFF timestamp excluded from comparison.

Checkpoint notes below retain the earlier frontier measurements.

Symbol input now has contextual typed variants in symbol-input.c, scope/record
registration in records.c, decoration in names.c, logical segment definitions
in segments.c, and storage selection in segment-storage.c. The per-function
driver is in functions.c. Backend pass dispatch is in backend-flow.c;
backend-symbols.c records module requirements. output.c constructs the named
output callbacks and module directives; coff-output.c adapts those callbacks
to the typed writer configuration, and coff-writer.c owns the encountered
vectors, scopes, packed relocation/line buffers, section contributions and
symbol records. coff-segments.c maps logical segments to COFF sections;
listing.c contains module listing logic. Initial module directives now pass
through the buffered data writer. Machine-symbol collection, initializer dependency discovery, pending-symbol output,
and machine/frame reset now reach the function reader. machine-opcodes.def records
the encountered instruction operand counts with named opcodes; it contains no
original pointer table or opaque data snapshot.

Function and expression input are now in function-input.c and tree-input.c.
expression-fields.c and control-input.c express the encountered stream recipes
as semantic field lists, with complete record/control dispatch in C. nodes.c
owns node/expression reuse; leaves.c owns canonical symbol and constant tables.
Per-function options and packed saved warning state now reach tree
canonicalization for the minimal case. canonicalize.c preserves expression
effects and source markers; tree-lowering.c reconstructs compound assignments,
volatile post-increment sequencing and bitfield paths. tree-references.c walks
the tree rewrites and propagates reference annotations. The 182
expression recipes, 47 control recipes and 182 semantic shape entries were
checked against the original classifications.

The current checkpoint has 236 reconstructed source functions and 297 recorded
matching trap frontiers. All 297 frontiers replay successfully with the matching binary CRT default. Callback stubs
remain trapped until reached; their signatures and extents are refined at that
point. The Makefile follows transitive local-header dependencies and continues
to compile and link entirely with the original toolchain.

Reconstructed logic is period C. The earlier pure tail-call initialization
adapters are ordinary C wrappers. The only functional inline assembly is the x87/ESI extended-float ABI, whose
raw ST(0) input and 80-bit memory result cannot be expressed by this compiler's
8-byte long double. The two narrow blocks are documented in extended.c.
Diagnostics use ordinary fixed three-argument __fastcall calls: ECX, EDX and
one callee-cleaned stack argument, matching original RET 4. Diagnostic handling
is still a named trapping stub; the current success-path cases and graph audit
do not validate diagnostic failure behavior. Unreached
function stubs deliberately use INT3 as required by the reconstruction process.

The leaf interner has an ordinary C varargs interface whose argument count
depends on the opcode. Original-compiler probes confirm an eight-byte raw
double struct returns in EDX:EAX. Twelve-byte Extended input uses __stdcall
so the callee pops the hidden result pointer (RET 4); a normal cdecl struct
return would use caller cleanup. Original and rebuilt calls/returns agree.
See cases/input-abi.json. The floating-input helper bodies remain trapped;
these static checks and the integer case do not validate floating input.

The tree classification table defines arity, self-contained records, second
operands, left-operand writes, side effects, associative/swappable operators,
scalar constant folding and additive operators as those uses are encountered. Its
182 entries match those original properties; it does not copy the packed
original opcode flags. Reference propagation records the four intrinsic
descriptors that carry an operand reference, without importing their name,
implementation or argument-format tables. Saved warning state uses packed
three-byte records. Its setter remains a trap; nonempty warning overrides
have not yet been exercised by the minimal case.

canonical-before.c and canonical-after.c implement the recursive canonicalizer's
pre-order and post-order hooks. Function-reference rewriting separates hidden
object arguments; temporaries.c owns the encountered temporary-register list.
tree-types.c implements full opcode typing, promotions, staged conversions,
type-stack records and conversion warnings. All 106 non-error opcode dispatch
arms match the original type-finishing table. The nine tree properties match
all 182 original opcode entries; the intrinsic call-class predicate matches
all 247 valid descriptors, with 110 selected entries. Ordinary format strings
remain at their points of use. The separate tree-construction and intrinsic
helpers remain traps until reached; the minimal case does not validate those
unexercised branches.

inline-calls.c and inline-candidates.c implement inline-call selection, recursive
growth budgets, body substitution and cleanup. Their optimizer flag is the
existing optimize_global variable at original 00080624. The minimal case has
no inline calls; substitution helpers still trap and have not been validated
on an actual inline expansion. memory.c releases the inliner's arena.

tree-optimizer.c orders operands by subtree keys and dispatches constant
folding. algebra.c implements the full algebraic simplifier and operator
identities. Its 28 opcode arms match the original dispatch, and all nine
encountered tree properties match 182 original entries. Arithmetic rewrite
helpers remain traps until reached; return-zero does not exercise those rules.

tree-cleanup.c preserves significant expression records before block splitting
in work-input.c. WorkItem is a typed 68-byte record with compiler size checks.
tree-scratch.h records the shared type-stack boundary and cleanup cursor as a
typed union: the original permitted boundary word aliases the cursor at
0007e840. No type-stack push/pop is exercised by this case.

Work preparation now includes parameter storage, target-tree lowering and block
selection. instruction-selection.c drives machine generation; machine-records.c
constructs the typed instruction and block variants. The case passes generate_work
and passes statement selection, value selection, return-zero folding, and
epilogue construction before reaching instruction side-effect analysis.
MachineInstruction and MachineBlockHeader are 32-byte tagged layouts; WorkItem's
body changes from an expression list to a selected block. Its use/definition
fields are register sets. Original-compiler size assertions cover these layouts,
eight-byte operands, twelve-byte call metadata and fifty-byte label symbols.

The optimizer, backend stage and default integer width reuse their original
shared option destinations. No duplicate phase or integer-width state is kept.
See cases/work-abi.json for ABI evidence and dynamic validation limits.

expression-selection.c implements the complete statement selector, using named
arithmetic/branch maps and shared physical register operands. machine-operands.c
owns the typed temporary-operand pool and operand equality; machine-operations.c
constructs instructions and tracks x87 depth. Its opcode classification and
stack effects match all 231 original operations. register-use.c analyzes
register lifetimes; selection-finishing.c repeats that analysis after rewrites.
All ten encountered tree properties match the 182 original entries. The opcode
definitions now name the implicit-register property required by this pass.
See cases/selection-abi.json for static evidence and dynamic validation limits.

instruction-rewrite.c implements the full 3,956-instruction machine rewrite
function as typed C. Its 20 opcode groups and 34 callee edges match the original;
no additional CRT copy helper was introduced by the original compiler. Register
slots have distinct full-DWORD and low-WORD views. The private helper at 00021790
reads ECX plus one stack argument and returns with RET 4; an unused EDX parameter
expresses that convention in ordinary __fastcall C. See
cases/instruction-rewrite-abi.json. The arithmetic and address-folding helper
bodies remain trapped until reached; this checkpoint does not validate their
unexercised branches or diagnostic failure handling.

The saved-register mask at original 0007c950 is shared by initialize_registers
and prologue/epilogue selection. Its initial 0xc8 saves EBX, ESI and EDI; it is
one variable in initialize.c. The original 0007caa4 mask starts at 7 for EAX,
ECX and EDX. Default alignment at 000806cc is the existing
default_segment_alignment value, distinct from code/section alignment at
000806d0. initialize_codegen establishes extended_zero at original 0007bf9c
and extended_one at 0007bf90. These identities are documented to keep later
passes from introducing duplicate state.

An independent debugger comparison at the earlier value-selector frontier confirms all fifteen
selected machine records, including the six prologue operations, agree between
both stopped processes. Only symbol/filename pointer addresses and use-link
addresses are normalized; referenced strings and record indices must agree.
This validates the saved-register mask in actual intermediate output. It does
not establish complete object equivalence or exercise the arithmetic rewrites.

value-selection.c implements the complete 1,441-instruction value selector,
including arithmetic, calls, conversions, constants and x87 selection. Its
32 callee edges and two dispatch-table extents match the original. The seven
floating-arithmetic map rows retain only contextual operator names and pop-form
operations. Operand width/replacement rules and branch simplification live in
instruction-uses.c. BitSet is a 12-byte sorted chunk list with a head sentinel;
bitsets.c reconstructs allocation, insertion and membership. Its 32 single-bit
masks are expressed as shifts. Function selection now constructs labels,
restores saved registers, selects the return register and closes both streams.
See cases/value-selection-abi.json for evidence and dynamic limits. The minimal
case exercises integer return-zero selection and folding. Other value-selector
branches still lead to named helper traps; floating arithmetic, calls, error
paths and general register allocation are not validated by this checkpoint.

A second independent stopped-process comparison at instruction_has_side_effects
checks all 32 selected records, including the return-zero MOV, register-set
chunks, label identity/reference count, and epilogue. Linked-list addresses and
symbol identities are normalized by record index or referenced name; register
sets are compared by their complete base/bit chunks. Both streams agree. This
still precedes machine lowering and object emission.

The minimal case now passes the unoptimized physical-register lowering and final
instruction-form passes. unoptimized.c retains separate physical/temporary live
sets, eight register mappings and their interference sets; bitsets.c also clears,
unions and recycles those sets. final-instructions.c reconstructs the complete
557-instruction finalizer, with its 16 original callee edges. frame.c resolves
frame-size pseudo operands and preserves the shared default alignment. It removes
the empty subtraction for this case. instruction-uses.c finds flag consumers,
retaining the special handling of repeat-prefixed string operations. Five named
machine-selection properties agree across all 231 original opcodes. See
cases/lowering-abi.json. General register spilling, alloca, stack probes and byte
rewrites retain unvisited helper traps and are not dynamically validated here.
The current frontier begins output layout; complete object output is still pending.

Output-layout analysis corrects two earlier provisional state names:
function_source_filename at original 0007e8a4 receives kind-5 filename records,
and function_source_line at 0007e8a8 receives current_source_location at the
kind-2 function marker. The begin_function callback takes that char * filename
and unsigned source line. The selection save mask remains 0007c950. The corrected build retains the original callback ABI and is included in the
checkpoint replay.

The corrected build passes all 289 frontier replays. An independent debugger
comparison at output-layout entry verifies all 31 finalized records, including
removal of the empty stack subtraction, SUB EAX,EAX for return zero, final
PUSH/POP operation IDs, label links/references and block register sets. This
checks machine forms before encoding; the 286-byte COFF comparison is pending.

Machine layout and common x86 encoding now pass the minimal case through to
COFF begin-function emission. machine-layout.c records code offsets, label
positions, function/prologue sizes and source positions, with complete alignment
and padding logic. FunctionSymbol retains its original 84-byte layout and has
named code-size/prologue/epilogue fields; DebugScope retains typed nested scopes.
The common encoder in instruction-encoding.c has 1,077 reachable original
instructions and shares sizing/emission logic. address-encoding.c selects ModR/M
and SIB bytes; operand-encoding.c resolves values and notifies the output format
through typed absolute, relative and segment reference callbacks.

encoding-recipes.c contains handwritten instruction recipes named by opcode,
form and modifiers. Instruction-family macros describe arithmetic, register,
fixed, memory and x87 encodings. All 231 extension bytes and six choices per
operation match the exact original encoding-table extents; packed form flags
are represented by named modifiers. The eight opcode properties used here match
all 231 entries. Padding instructions are explicit small x86 byte sequences
introduced by the encoder's NOP case. All 16 encoder diagnostic codes agree
with their original sites; this is a static check, not failure-path validation.
See cases/encoding-abi.json. The minimal case exercises register instructions
and a near jump; floating/alternate/address forms still reach unreconstructed
helpers. Full instruction emission and complete object comparison remain pending.

This encoder/layout checkpoint passes a clean original-toolchain build, all 236
graph audits and all 297 frontier replays. Six changed C modules compile cleanly
with /W3. An independent stopped-process comparison at COFF begin-function entry
matches all 31 machine records, including cached instruction lengths. Both
processes report code offset 0, code size 18, prologue size 6, epilogue offset 0,
frame allocation 0, filename test.c and source line 1. The callback receives the
same logical symbol/filename in ECX/EDX and stack arguments 0 and 1. Encoding
for sizing has passed; the complete emitted object is still pending.

COFF function setup now preserves the source-file cache, optional per-function
CodeView/FPO sections, symbol reachability, weak fallbacks and export directives.
coff-functions.c keeps these states beside their users. names.c formats existing
decorated names and internal numbered names in the shared object-name workspace.
coff-writer.c creates typed, packed 18-byte COFF symbols, chooses short or string
table names, and resolves pending function relocations. Line information is
controlled by the writer's debug byte, independently of its feature flags.

At assembly-listing entry, an independent debugger compares all seven writer
symbol entries. Both contain the test.c file record, .drectve and .text section
records and their auxiliary placeholders, followed by the exact 18-byte _main
symbol: value zero, contribution 2, type 0x20, storage 2, no auxiliary record.
The section flags, contribution numbers and symbol indexes also match. See
cases/coff-symbol-abi.json. Optional debug/FPO output, weak/exported symbols,
long names and diagnostic branches have complete parent bodies and named
trapping helper calls but are not dynamically validated by this minimal case.
Complete byte emission and the final object comparison remain pending.

This COFF symbol checkpoint passes a clean original-toolchain -j2 build, all
244 graph audits and all 306 recorded frontier replays. Five changed C modules
compile cleanly with /W3. The active frontier is write_function_listing; the
listing-disabled path still precedes machine byte emission and final COFF output.

Machine byte emission and function cleanup now reach COFF module closure.
machine-emission.c reconstructs the complete record-dispatch body and invokes
typed segment, byte, source-line, source-file and function callbacks. listing.c
also contains the complete function-listing body, with named helper traps for
the unvisited formatting/source paths and a typed twenty-file source cache.
Work cleanup clears optimizer trees and pooled bitsets before resetting arenas,
recycles three interned-leaf tables, then disconnects per-function symbol state.
Shared roots such as temporary_registers and free_bitset_nodes are reused.

Independent stopped-process inspection at COFF end-function entry matches all
18 .text bytes (558bec5356572bc0e9000000005f5e5bc9c3), all 38 .drectve bytes,
both scope sizes and all seven symbol entries. This proves emitted section data;
COFF header/symbol serialization and the complete object comparison are pending.
See cases/emission-abi.json for callback ABIs, state provenance and limits.

Reaching cleanup exposed an older mask error: release_expression's earlier
extent ended at 0004fd10 and exposed the adjacent free_work body. Primary control
flow ends at its RET at 0004fcfd; the corrected logical extent ends 0004fd00.
Two still-provisional extents were tightened as well. compare.py now rejects all
overlapping driver extents before masking. The corrected masks restore the
matching free_work frontier; that function is reconstructed independently.

This emission/cleanup checkpoint passes all 259 call-graph audits and all 321
frontier replays after the extent correction. All 560 driver ranges are disjoint.
Twelve changed implementation modules compile cleanly with /W3; deliberate INT3
stubs retain the expected no-return diagnostics at that warning level. The active
frontier is coff_finish_module, with complete object serialization still pending.

The captured minimal-C backend case now completes. The masked original,
reconstruction and unmodified original each exit zero with empty output streams.
All produce the expected 286-byte test.obj. Every byte agrees after clearing only
the four-byte COFF TimeDateStamp at offset 4; normalized SHA-256 is
a99b80966425c2b3ed532f78e939c91f018e6f70682f52525fbe6b579fb79929.

coff-finalize.c writes typed file/section directories, file and section auxiliary
symbols, ordinary symbol records and the string table. coff-writer.h checks
the original compiler's packed record sizes. Section data preserve the original
cumulative physical-address field, including 38 for .text. The field at 00085198
is now named coff_reserved_sections; 000851b0 is coff_data_offset. Relocation and
line counts retain separate 16-bit fields. No output bytes are supplied by the
fixture or a linked snapshot; they are generated by the reconstructed pipeline.

The older CRT ftell body is also reconstructed as ordinary C in runtime.c. Its
complete implementation accounts for buffered binary/text streams and translated
newlines; the minimal case exercises buffered binary writes. Its extent ends at
00071e90, preserving the adjacent fgets trap. compare.py now checks driver ranges
against identified original CRT ranges as well as other driver ranges. The
normal exit hooks preserve incomplete-output cleanup and listing finalization;
failure/optional listing paths are not dynamically exercised by this case.
See cases/coff-serialization-abi.json for extents, record/state provenance and
remaining validation limits. Optimized Win32 hello remains the next case.

The completed minimal case passes a clean original-toolchain make -j2 check,
all 275 function-graph audits and all 352 recorded frontier replays through final
exit/output comparison. Seven final implementation modules are /W3 clean.
The queued optimized hello case follows this frozen 352-frontier history.

The optimized Win32 hello profile is active. Use make hello-frontier,
make hello-replay and make hello-check (or compare.py --profile hello).
Its history begins after minimal frontier 352; make replay retains the frozen
minimal history and successful final comparison. Both profiles pin the manifest,
input hashes and expected object hash and preserve the single-entry environment.
Successful comparisons write build/validation-minimal.json or
build/validation-hello.json. The first hello frontier is new_initializer.

Hello now passes initializer allocation, the full initializer-body decoder,
exact byte-span input, sorted insertion, constant classification and module
directive input. Initializer records use a checked 28-byte typed union. An
independent debugger comparison matches both roots (IDs 9279 and 9290), their
record chains and all 26 string-data bytes. All six hello frontiers replay,
all 281 source graphs agree, five changed modules are /W3 clean and the complete
minimal output/exit regression passes. That initializer checkpoint ended at
symbol-storage allocation; its object comparison was still pending. See
cases/hello-initializers-abi.json for provenance and unvisited-path limits.

Hello additionally passes variable storage allocation, downward local-frame
allocation and alignment, expression-list insertion, backend-to-frontend type
classification and explicit scalar conversion. The incoming-parameter cursor
at 0x806e0 is now named parameter_stack_offset. A paired mismatch exposed an
inverted unary-completion condition in read_expression; correcting that test
restores the original 27 first-function expression roots. An independent
debugger comparison agrees on all 97 visited nodes, including argument chains
and switch operands. Only the debugger entry address and a child allocation
pointer are normalized in that tree receipt.

The current hello frontier is fold_constant_conversion. Its helpers retain
named typed traps; complete hello object output remains pending. See
cases/hello-storage-abi.json for function extents, the reader correction and
failure/alternate-branch validation limits.

This storage/tree checkpoint has 287 reconstructed functions and 364 accepted
frontiers. The final build passes all 352 frozen minimal frontiers, all 12
hello frontiers, the graph audit, seven changed modules compiled /W3, and the
complete minimal object/exit comparison. Extent-overlap checks run before each
mask. The hello object remains pending at fold_constant_conversion.

The optimized hello case now completes all three global tree passes and reaches
unused-expression cleanup. Twelve additional functions preserve the original
fixed-point drivers, recursion order, scalar-conversion graph and operator
rewrites (1,441 native instructions). The floating frontend flag at 0x83150
and precision optimization at 0x80698 are tracked separately; the new floating
rounding/rewrite guards use the precision flag. Typed trapping helpers retain
unvisited arithmetic and extended-conversion branches.

An independent comparison at the end of canonicalization/inline expansion
matches all 29 expression records and 99 visited nodes of _HelloWindowProc@16,
including symbol IDs/names/offsets, scalar leaves and node self-reference links.
Only the debugger exception location is normalized. Detailed extent, ABI,
flag-audit and branch-coverage evidence is in cases/hello-global-tree-abi.json.

The global-tree checkpoint has 299 source functions and 376 accepted frontiers.
The final build passes the complete 352-frontier minimal history, all 24 hello
frontiers, the graph audit, both new modules compiled /W3 and the full minimal
object/exit regression. Hello output remains pending at cleanup_unused_tree.

Hello now passes expression cleanup, tail-call scanning, the global-work driver,
block partitioning and its shared list constructor. The constructor retains
separate typed work/expression cursors for every command. WorkItem now names its
previous-block link at offset 4 and containing region at offset 24; the original
68-byte layout is checked. The state at 0x7ba80 is the exit WorkItem pointer,
assigned on opcode 0x47, and is distinct from the entry pointer and region tree.
Label hash heads, the current alias group and the detached input cursor have
separate owners. Typed trapping helpers retain the remaining flow analysis.

An independent capture after all global tree passes, cleanup and tail-call
scanning still matches the first function's 29 roots and 99 visited nodes,
including captured scalar/symbol/self-reference fields. Only the debugger
exception address is normalized. The first hello function does not exercise
self-recursive tail-call rewriting or the list constructor's alternate modes;
those paths are source/ABI/graph audited rather than claimed as runtime passes.
See cases/hello-work-abi.json for exact extents, layouts and validation limits.

This checkpoint has 309 source functions and 386 accepted frontiers. All 352
minimal and all 34 hello frontiers replay with matching logical traps. Seven
changed modules compile /W3 clean; the graph audit and full minimal object/exit
regression pass. The hello object remains pending at reference_flow_label.

Hello now passes symbol-identity label lookup, descriptor/link allocation,
reference deduplication, recursive adjacent-label folding, alias definition and
canonicalization. FlowLabel has a checked 20-byte layout and FlowLink has a
checked 12-byte layout; neither is supplied by a data snapshot. Recycled links
retain their flag byte, as in the original allocator. Label descriptor, hash,
block and reference pointers remain distinct typed fields.

At prepare_flow_blocks, an independent debugger capture matches all eight
partitioned blocks, 25 expressions and 95 visited nodes of the first function.
This includes block order, entry/exit identity, previous-link consistency, flags,
region presence and captured expression/tree fields; only the debugger exception
address is normalized. See cases/hello-labels-abi.json for exact ABI/extents and
remaining branch limits. Protected-label and scope helpers remain named traps.

The label checkpoint has 319 functions and 396 accepted frontiers. All 44 hello
frontiers replay, all graph edges agree, both changed implementation modules
compile /W3 clean, and the complete minimal object/exit regression passes. The
352-frontier minimal history was fully replayed at the preceding work checkpoint.
Hello output remains pending at prepare_flow_blocks.

Hello now passes complete block-edge preparation, branch-target normalization,
ordinary successor/predecessor insertion and link recycling. Successor and
predecessor fields are typed separately; inline-assembly symbol visitors retain
the original two register arguments plus callee-cleaned stack context argument.
Source exception scopes and optimizer regions have separate types and ownership.
Protected-scope, assembly and malformed-label paths retain named typed traps.

At refresh_flow_edges, which simplifies/coalesces the block chain, the independent
capture matches eight blocks, nine ordered successor links, nine ordered
predecessor links, 25 expressions and 95 visited nodes of the first function.
Targets, edge flag bytes, symbol fields, block order and entry/exit identity agree;
only the debugger exception address is normalized. See cases/hello-edges-abi.json.

This edge checkpoint has 324 source functions and 401 accepted frontiers. All 49
hello frontiers replay, all graph edges agree, both changed implementation
modules compile /W3 clean, and the complete minimal object/exit regression passes.
Hello output remains pending at refresh_flow_edges.

Hello additionally passes block simplification, numbering, reachability marking
and unreachable-block cleanup. The 245-instruction routine at 0x59a30 is now
named simplify_flow_blocks: it removes unused labels and fall-through branches,
inverts conditional branches when appropriate and coalesces compatible adjacent
blocks. The 51-instruction driver at 0x5a5d0 is remove_unreachable_work. The full
129-instruction removal body preserves edge unlinking, six dynamic bitset slots,
region landmarks, label bookkeeping and diagnostics; optional diagnostic paths
remain unvisited rather than claimed as passing while their helpers trap.

WorkItem names its short block number and uses an explicit union for the phase-
specific traversal mark/dominator set at offset 20. Six runtime BitSet pointers
occupy offsets 36 through 56. FlowRegion has a typed encountered prefix for
sibling/child/parent links, entry/header/last work blocks and flags. Its original
allocator reserves 44 bytes; fields beyond the encountered prefix are deferred.
These are contextual dynamic structures, not image data or snapshots.

Independent captures agree after simplification (8 blocks, 24 expressions,
94 visited nodes, 9 ordered edges each direction) and after reachability removal
(7 blocks, 23 expressions, 93 visited nodes, 8 edges each direction). Only the
debugger exception address is normalized. Detailed function extents, ABI,
branch limits and region-storage provenance are in cases/hello-blocks-abi.json.

This checkpoint has 331 reconstructed functions and 408 accepted frontiers.
A clean original-toolchain make -j2 check passes, followed by all 352 minimal
and all 56 hello frontier replays. All source graphs agree and five changed
implementation modules compile /W3 clean. Hello output remains pending at
merge_flow_blocks, the physical block-ordering pass at 0x5a540.

Hello now passes physical block ordering, recursive successor traversal,
ordinary-predecessor counting, stable edge sorting and explicit fall-through
bridge insertion. The routine at 0x5a540 is now named order_flow_blocks. Its
reverse-postorder chain head at 0x7bec0 is a contextual WorkItem pointer, not a
counter. Ordinary and protected predecessors remain distinct when computing
sort priorities. Branch insertion exercises the shared constructor's SNRnlTL
commands and preserves the original labels, reference counts and edge updates.

The second simplification pass also exercises block merging and the region-
landmark transfer adapter. The first function has no regions, so its deeper
region-last replacement helper remains a named typed trap. Synthesizing a new
target label also remains behind its typed trap; the dynamic bridge path uses
an existing label. See cases/hello-order-abi.json for full extents and limits.

An independent capture before dominator construction matches six numbered
blocks, 21 expressions, 91 visited nodes and seven ordered edges each direction.
This additionally checks the null head predecessor, all other previous links,
entry/exit identity, scope numbers, tree/symbol fields and label reference
counts. Only the debugger exception address is normalized.

The ordering checkpoint has 339 reconstructed functions and 416 accepted
frontiers. All 64 hello frontiers replay, all source graphs agree, four changed
modules compile /W3 clean, and the complete minimal object/exit regression passes.
The frozen minimal history was fully replayed at the preceding clean-build
checkpoint. Hello output remains pending at compute_work_dominators.

Hello now completes dominator construction. The driver initializes the entry's
set to block zero, initializes other blocks to the universal set and repeatedly
intersects predecessor sets before adding each block itself. Sparse bitset
initialization, universal fill, intersection, equality and in-place assignment
preserve the original call graph and node-recycling behavior. The allocator
interface retains the caller's capacity register even though the native helper
ignores that capacity for this sparse representation.

An independent capture before region discovery matches all six dominator
sentinel/member chains: 12 chunks with exact signed bases and 32-bit membership.
The six-block graph, 21 expressions, 91 visited nodes, edge orders, numbering,
label reference counts and captured symbol fields still match. Only the debugger
exception address is normalized. Full-word fills and copy-tail assignment
remain unvisited in this first function; their downstream copy helper retains
its typed trap. See cases/hello-dominators-abi.json for extents and branch limits.

The dominator checkpoint has 345 source functions and 422 accepted frontiers.
All 70 hello frontiers replay, all graph edges agree, both changed implementation
modules compile /W3 clean, and the complete minimal object/exit regression passes.
Hello output remains pending at find_flow_regions.

Hello now passes region discovery and its candidate predecessor scans. The full
156-instruction parent constructs natural-loop preheaders and irreducible ranges,
partitions predecessor chains, redirects branch labels and links nested region
records. Its allocator reserves the native 44 bytes even though only the region
prefix is currently encountered. A named local enum expresses this allocation
extent; no unknown trailing storage or image snapshot is copied into C.

Candidate scanning uses separately owned WorkItem pointers at 0x7becc and
0x7bec8, marks each reachable block once and clears all temporary marks before
advancing to the next candidate. The first function has no loop regions, so
preheader creation, nested region assignment and their typed helper traps remain
unvisited. The redirect helper's fixed three-argument fastcall ABI has one
callee-cleaned stack argument (RET 4), expressed as ordinary typed C calls.

An independent post-discovery capture agrees on the null region roots, cleared
visit bits, six numbered blocks, 12 dominator chunks, 21 expressions, 91 visited
nodes, edges, symbols and label uses. Only the debugger exception address is
normalized. See cases/hello-regions-abi.json for exact extents and branch limits.

This checkpoint has 350 source functions and 427 accepted frontiers. All 75
hello frontiers replay, all graph edges agree, the new module compiles /W3 clean,
and the complete minimal object/exit regression passes. Hello output remains
pending at analyze_flow_regions (0x59d30), the 319-instruction loop-rotation pass.

The first hello function now completes the entire global-work driver and reaches
the optimizer proper. The full 319-instruction loop-rotation routine is named
rotate_flow_regions; its complete C body preserves test movement/copying, label
retargeting, exit bridges and nested-region order. finish_flow_regions inserts
synthetic boundaries and increments the unsigned short region_depth at offset
62. The separate signed field at offset 64 is now named exception_scope.

The routine at 0x59720 is insert_function_boundaries. It restores control codes
0x0f/0x10 after flow edits, retains optional CodeView codes 0x11/0x12 and splits
the exit block when needed. Literal constructor commands stay at their uses.
The original entry-search fallback repeats head->next; that behavior is retained.
The current first function exercises the normal entry/in-place exit insertion,
while loop bodies, split-exit and optional debug branches remain unvisited.

Before optimizer unmasking, its provisional no-argument signature was corrected
to void __fastcall optimize_work(WorkItem *). The native parent retains the work
head in ECX at 0x4c3fa..0x4c40f, and the callee consumes it at 0x5ba72. The typed
caller now passes work explicitly. Independent stopped-process capture verifies
ECX equals the work head in both children; optimizer behavior remains trapped.

Final first-function global-work state agrees on six numbered blocks, 23
expressions, 93 visited nodes, 12 dominator chunks, both edge lists, depth/scope
fields, markers, symbols and label reference counts. Only debugger exception
address is normalized. See cases/hello-global-work-abi.json for extents, ABI
correction evidence and unvisited-path limits.

This checkpoint has 353 reconstructed functions and 430 accepted frontiers.
After the caller correction, all 352 minimal and all 78 hello frontiers replay.
All source graphs agree, six changed modules compile /W3 clean and the complete
minimal object/exit regression passes. Hello output remains pending at the typed
optimize_work entry, 0x5ba70.

The optimizer now completes value initialization and reaches reference-map
construction. The driver, initializer and recursive value walk preserve their
separate graph vertices. The walk has a fixed three-argument fastcall ABI with
one unused callee-cleaned stack slot (RET 4), expressed in ordinary typed C.

Memory accesses have a contextual NODE_MEMORY_ACCESS property. Its exact 18
members agree with the original across all 182 opcode entries; the existing
side-effect, assignment-target and interned-identity properties were also checked
across the full opcode range. This extends the named semantic recipes rather
than copying the native packed table. WorkValue defines only its encountered
16-byte prefix; the interner retains the original 24-byte allocation and leaves
the final two words uninterpreted. Hash buckets, classification sets and creation
links are built at runtime. Ordinary literals remain at their uses.

The interner normalizes memory operators to loads, reuses equal address/type
values and classifies new identifiers into the native bitsets. Tree cloning,
conversion stripping and recursive equality are reconstructed individually.
Equality retains its shared mode wrapper, assignment/load equivalences, volatile
checks and floating-comparison helper trap. The optimizer's later reference,
rewrite and region passes remain named typed traps until encountered.

Independent stopped-process comparison before build_work_references matches all
64 value buckets, ten typed value records, creation links and identifiers, 22
canonical-tree nodes and eight classification-set chunks. The existing six
blocks, 23 expressions, 93 work-tree nodes, edges, symbols and dominators still
match. Debugger exception addresses are normalized; non-self canonical memory
pointers are compared by their value identifiers, while canonical self pointers
remain explicitly checked. Both stopped children pass the same work head in
ECX. No loop regions occur in this first function. Strict/volatile and floating
equality branches remain outside this successful prefix's dynamic coverage.
See cases/hello-work-values-abi.json for extents, data provenance and limits.

This checkpoint has 366 reconstructed functions and 443 accepted frontiers.
All 352 minimal and all 91 hello historical frontiers replay. All source graphs
agree, six changed implementation modules compile /W3 clean and the complete
minimal object/exit regression passes. Hello output remains pending at
build_work_references, 0x5bda0.

The first reference-building pass now completes. Its 21 reconstructed functions
cover 1,674 reachable native instructions, including predecessor availability
merging, block/tree reference analysis, value substitution, expression numbering,
delayed memory effects, alias policy and the separate record/table free lists.

WorkItem now states explicitly that offsets 0x1c/0x20 hold reference-table and
successor-count fields during optimization, then uses/definitions bitsets during
instruction selection. Its size remains 68 bytes. FlowRegion's trailing loop,
reference, candidate and availability pointers complete its native 44-byte
layout. WorkReference is a typed 12-byte next/node/value record, and its 128-byte
table is a union of 32 bucket heads and a pool link. The numbered-expression
index uses distinct typed 20-byte records with structural and identifier chains,
a canonical node, cached temporary load and an eight-byte use-list type.

Two contextual alias-policy matrices define exactly 16 one-byte cells each.
Each cell names the predicate performed by the effect helper: escaped address,
same symbol, same address, nonprivate value, same/unknown symbol or unconditional
aliasing. Both compiled matrices match their complete native extents exactly,
ending before adjacent state at 0x837f0. No unrelated image data is copied.
Reference-root and value-expression properties refine the named opcode recipes;
all 182 entries agree with the native 90/53 membership sets. Ordinary diagnostic
strings remain literals at their uses.

flag_killed_work_references replaces its provisional pruning name: it marks
references that depend on killed canonical memory values without removing them.
The separate invalidator removes aliasing loads, records their canonical IDs and
leaves detached record storage to its arena. Allocators retain native distinctions
between fresh arena storage and explicitly cleared recycled records.

An independent stopped-process capture before expression-cache cleanup matches
all 267 normalized lines: six blocks, 23 expressions, 93 work-tree nodes, ten
memory values, six numbered expressions and their identifier/use chains, two
map-entry occurrences, and ten flow-set chunks. It visits 143 total tree nodes
across the work and indices. Canonical-operand pointer identity is checked, but
all such fields are zero here: the numbered expressions are simple loads, so
non-leaf canonical sharing remains unvisited. The capture normalizes debugger
exception addresses, canonical-memory pointers by value ID and list/symbol
pointers by their contextual identity. Region paths and final reference marking
remain outside this first pass's dynamic coverage. Exact extents, data provenance,
ABI details and limits are in cases/hello-work-references-abi.json.

This checkpoint has 387 reconstructed functions and 464 accepted frontiers.
All 352 minimal and all 112 hello historical frontiers replay. All source graphs
agree, five changed implementation modules compile /W3 clean and the complete
minimal object/exit regression passes. Hello output remains pending at
clear_work_expression_cache, 0x5bb20.

The first memory-value rewrite now completes and reaches region hoisting.
Eleven newly reconstructed functions cover 802 reachable native instructions:
numbered-cache cleanup, canonical alias-set construction, block memory summaries,
backward memory liveness and their sparse-bitset operations. WorkValue's full
24-byte layout now names its last local store pointer and alias-set pointer.
Interning leaves those two fields untouched until their owning pass initializes
them. Block flow-set indices have explicit names for reads, stores, unread stores,
live-in and live-out. All these sets and pointer chains are built at runtime.

Memory liveness solves uses | (live-out - definitions) backwards to a fixed
point, keeping externally observable memory values live at the exit. The transfer
helper has a fixed four-argument fastcall ABI with RET 8; its provisional return
type was corrected to void before unmasking. Union/difference assignment helpers
are fixed three-argument fastcall functions with RET 4 and no return value.
Their C preserves native sentinel handling, allocation order and chunk recycling,
including difference allocation before its zero-result test. Cache cleanup
releases indexed roots shallowly before resetting arena four.

Independent capture after the first rewrite and dominator refresh matches all
259 normalized lines: six blocks, 23 expressions, 93 work-tree nodes, ten canonical
memory values, 20 alias-set chunks and 45 block flow-set chunks. Canonical memory
pointers are compared through their indexed value identity, including any operator
whose opcode was converted during rewriting. Numbered/reference storage has been
reset and is no longer followed; last-store slots are no longer logically live
after cleanup and are not interpreted. The first function does not actually
remove a dead store or contain loop regions, so those paths retain their named
typed downstream traps. See cases/hello-memory-flow-abi.json for exact extents,
ABI corrections, ownership details and dynamic coverage limits.

This checkpoint has 398 reconstructed functions and 475 accepted frontiers.
All 352 minimal and all 123 hello historical frontiers replay. All source graphs
agree, three affected implementation modules compile /W3 clean and the complete
minimal object/exit regression passes. Hello output remains pending at
hoist_region_values, 0x5c020.

Run make coverage for the accepted original code-extent union, including source,
shared text data, CRT and import thunks. The report unions overlapping intervals,
clips them to original code sections using VirtualSize (raw size if zero), and
verifies that union against the actual INT3 masker. The memory-flow checkpoint coverage is 126,319
of 399,693 original code bytes, or 31.6040 percent. This is code-extent coverage,
not branch coverage; the original target hash is included in the JSON report.

The first work-preparation pass now reaches switch emission. Ten additional
functions reconstruct 513 reachable native instructions: region-motion setup,
numbered-expression rewriting, temporary storage allocation, the postfix tree
builder, switch setup and range coalescing. The first function has no loop regions
or numbered expressions requiring materialization; their named helper traps stay
in place for later cases.

The builder implements its complete character recipe language in C switch
statements, including argument lookahead for O and S and call-visible operand
stack updates for recursive helpers. Its zero-initialized typed 64-pointer stack
occupies the contextual original range 0007e718..0007e818. The adjacent
current_expression object establishes its upper boundary. The original character
jump table is analyzed and included in the function's mask extent; it is not
copied into reconstruction source. The current recipe DNTS deep-copies the
reference, constructs a load and inherits the selector's type/symbol metadata.

Kind-19 temporary descriptors have a packed 13-byte layout. Their allocator
aligns bytes using the common argument/temporary alignment, reserves local frame
storage and then stores that negative frame offset. The reference wrapper keeps
its original two-register signature even though its second argument is unused.
All generic symbol interfaces receive an explicit descriptor cast. A duplicate
provisional allocator record was rejected by preflight before execution and was
removed in favor of the existing one-argument ABI.

The provisional lower_expression_stack function is now named lower_switch_dispatch:
it retrieves the saved selector, updates memory-store flow, coalesces adjacent
case ranges and emits dispatch tests before releasing the old range/selector
storage. Independent typed capture at the emission frontier matches all 267
normalized lines, including temporary frame offsets and the selector tree held
by its label. Kind-19 descriptors are interpreted through their actual fields;
label selector pointers are traversed as trees instead of compared as heap
addresses. Empty-region motion, unvisited builder recipes, diagnostics and
multi-range joining have no claimed dynamic branch validation. See
cases/hello-tree-builder-abi.json for the exact extents, ABIs and limits.

The graph auditor now decodes past a declared function end before checking the
instruction boundary. This rejects extents that truncate a multi-byte operand,
including RET immediates. All 485 accepted original and reconstructed extents
pass this check, including runtime/import bodies; all 408 reconstructed source
function graphs agree. Seven changed implementation modules compile /W3 clean.

This checkpoint has 408 reconstructed functions and 485 accepted frontiers.
All 352 minimal and all 133 hello historical frontiers replay. Minimal-C still
produces its exact captured object except the four timestamp bytes. Optimized
hello remains pending at emit_switch_dispatch, 00067ed0. The verified accepted
code-extent union is 128,239 / 399,693 bytes, or 32.0844 percent; this is not branch
coverage. Source, shared text data, CRT and import thunks are unioned and clipped
against the original code VirtualSize and checked against the actual masker.

The first hello function now completes work preparation and reaches global
allocation at finish_global_work, 00062100. Seven more functions recover 442
reachable native instructions: dispatch strategy selection, range costing,
linear switch tests, common scalar type selection, stable-head expression
insertion, assignment lowering and conversion stripping. The switch uses two
equality tests and a default branch; the dense-table and recursive-split
callees remain named typed traps.

Switch selection counts singleton tests once and interval bounds twice. It
chooses linear tests below six comparisons and uses the original four-to-one
span threshold for a dense table. Expression insertion preserves the original
head record's address, moves its old metadata to the new successor and clears
the inserted head's metadata. Integer promotion retains signed-width comparisons
and the original selector/offset-pointer type precedence. Assignment lowering
has its complete operator cases and fixed typed helper ABIs, while the first
function currently takes the non-compound path. The retained selector tree is
accessed through its actual label prefix field, rather than an Expression view.

Independent typed state at allocation entry matches all 277 normalized lines.
The generated selector loads retain copied detail pointers to the saved selector;
those identities are checked against switch_selector without following released
storage. Opcode-one no-op nodes retain obsolete payload fields, which are not
interpreted as values. These checks do not claim coverage for alternate switch
strategies, compound updates, floating promotion or diagnostics. Exact extents,
ABI details, receipt hashes and limits are in cases/hello-work-prepare-abi.json.

This checkpoint has 415 reconstructed functions and 492 accepted frontiers.
All 352 minimal and all 140 hello historical frontiers replay; all source graphs
and accepted instruction boundaries agree. Three affected modules compile /W3
clean and the complete minimal object/exit regression passes. The independent
277-line capture includes 26 expressions and two preserved selector-copy pointer
identities. Verified accepted code-extent coverage is 129,647 / 399,693 bytes,
or 32.4366 percent, including source/shared text/CRT/import thunks with union,
clipping and an actual-mask cross-check. This is not branch coverage. Hello
output remains pending at finish_global_work, 00062100.

The first global allocation initializer now completes and reaches connected-range
collection. Thirteen more functions recover 1,012 reachable native instructions:
the complete allocation driver/initializer, canonical address hashing, value
interning, load scoring, call detection/effect construction, shared sparse-bitset
enumeration and the forward reaching-definition solver. The driver retains
named typed traps for later range, liveness and register-assignment stages.

AllocationValue is a runtime-built 28-byte record, keyed by canonical address
operand identity in 32 pointer buckets. Its six recovered records own six initial
definitions and two store definitions here. Store/call kill sets expand through
a temporary typed definition-to-value index, then block out sets solve
(in - kills) | gen to a fixed point. Named flow-set indices identify this phase's
gen, kill, in and out sets. Expression offset eight now has a typed call-effect
bitset union member while retaining the original 16-byte layout; an original-
compiler sizeof assertion verifies that ABI. All new counters, pointers and
arrays belong to the pass that constructs them; no image data is copied.

The load scorer can rank up to 500 observable values and retains its native
conditional arena-reset order. The sorting branch has a named cdecl comparator
trap and uses the original qsort library implementation, identified with its
relative relocations. The sparse iterator has the original shared chunk/bit
cursor and a distinct lowest-set-bit helper, including the original zero-word
result and zero-chunk behavior. Hashing uses numeric symbol identities and
frame-temporary offsets; floating constant keys are zero.

Independent state after initialization matches all 309 normalized lines across
the six allocation records, all 32 buckets, eight definition IDs, owning/global
definition sets, call effects, block reachability, trees and existing memory
values. These are direct local/parameter values: calls are found, but no tracked
observable value requires a nonempty call-effect set here. Sorting, range
creation/merging, diagnostics, floating register tracking and exception filtering
remain outside this checkpoint's dynamic coverage. Temporary arena-four indices
have been released and are not followed; not-yet-owned register-set slots are
not interpreted. See cases/hello-definition-flow-abi.json for extents, typed data
provenance, receipt hashes and validation limits.

This checkpoint has 428 reconstructed functions and 505 accepted frontiers.
All 352 minimal and all 153 hello historical frontiers replay. All source graphs
and accepted instruction boundaries pass audit; four affected implementation
modules compile /W3 clean. Minimal-C still produces its exact captured object
except the four timestamp bytes. Verified accepted code-extent coverage is
133,023 / 399,693 bytes, or 33.2813 percent, including the union of source,
shared text data, CRT and import thunks, clipped to original code and checked
against the actual masker. This is not branch coverage. Hello output remains
pending at collect_allocation_ranges, 00062960.

Connected allocation-range collection now completes, and the driver assigns six
virtual registers before reaching register rewriting. Four more functions cover
269 reachable native instructions: connected-component collection, the range
factory, sorted-bitset overlap testing and register-variable table reset. Each
observed value currently has one range; repeated accesses extend its block bounds.
The complete collector also preserves component merging and the original
once-per-block/once-per-value initialization diagnostics, with named typed traps
for unvisited release, region and warning helpers. records.h supplies the exact
cdecl warning declaration; a /W3 check caught that missing include before the
checkpoint replay was restarted with the corrected declaration.

The register-variable map has three classes with 32 record-pointer hash heads
each, matching the 96-pointer clear and the constructor/lookup's class and bucket
indexing. This is a typed runtime table; its record remains forward-declared until
the constructor is reached. Range creation initializes only its short register
number; the collector owns bounds, definition sets, hints and linkage. No opaque
record image or data-section content is copied.

Independent state at register-rewrite entry matches all 344 normalized lines:
six range bounds and definition sets, registers 4020 through 4025, the next
register counter 38, per-value/global register sets and all 96 class/bucket heads,
plus prior work/tree/definition state. These tables are fresh and empty here.
Range-component joining/recycling, initialization diagnostics, nonzero range
hints and populated-table reset remain outside current dynamic validation. See
cases/hello-allocation-ranges-abi.json for exact extents, ABI evidence and limits.

This checkpoint has 432 reconstructed functions and 509 accepted frontiers.
All 352 minimal and all 157 hello historical frontiers replay. All source graphs
and accepted instruction boundaries pass audit; three affected implementation
modules compile /W3 clean after the diagnostic declaration correction. Fresh
minimal-C object/exit regression passes. Verified accepted code-extent coverage
is 133,951 / 399,693 bytes, or 33.5135 percent, including source/shared text/CRT/
import thunks with union, clipping and the actual-mask cross-check. This is not
branch coverage. Hello output remains pending at rewrite_allocation_registers,
000627a0.

Register rewriting, backward liveness, forward reference history and exception
exclusion now complete. Four functions cover 394 reachable native instructions.
Direct accesses retain their canonical address in Node.detail and receive the
connected range's A2 virtual-register leaf. Register reference/upward-use/kill
sets reuse the earlier definition slots, then history-out replaces the released
upward-use set. Both fixed-point solvers preserve null-versus-empty sentinel
behavior, iteration direction and successor/predecessor union order.

Independent state at transfer insertion matches all 355 normalized lines,
including 14 virtual-register leaves and their exact saved canonical-address
identities, six blocks' completed live sets, history sets and prior allocation
state. Address identities are matched through the actual owning allocation
value's address operand; structural similarity is insufficient. No protected
exception edges or regions occur here, so actual exclusion/restoration remains
unvisited and its named tree-restoration helper still traps. Missing-range
internal-error diagnostics also remain unvisited traps. See
cases/hello-register-flow-abi.json for extents, ABI evidence, receipts and limits.

This checkpoint has 436 reconstructed functions and 513 accepted frontiers.
All 352 minimal and all 161 hello historical frontiers replay; all source graphs
and accepted instruction boundaries pass audit. The affected implementation
compiles /W3 clean, and the fresh minimal-C object/exit/streams regression passes.
Verified accepted code-extent coverage is 135,199 / 399,693 bytes, or 33.8257
percent, including source/shared text/CRT/import thunks with union, clipping and
the actual-mask cross-check. This is not branch coverage. Hello output remains
pending at assign_global_registers, 000613f0, whose role is transfer insertion.

The global transfer/range pass now completes. Eleven more functions cover
1,034 native instructions: scalar transfer insertion and placement, virtual
register-use walking, live-range extension and recording, stack-home and
parameter classification, both register-variable constructors and sparse-set
emptiness. The transfer pass builds typed arena-one register/value/range and
definition/block indices. Six native 52-byte register-variable records retain
numbered-block bounds, type, parameter/observable flags and symbol/offset homes.
Unknown allocation fields remain named by offset until their consumers are
encountered; all are initialized by the factory, with no data-section image.

Tracing the selection handoff exposed two early duplicate-state interpretations.
The candidate set and former selection_anchor are one original pointer at
0007b9e0. Likewise the old optimizer-tree cache is the canonical work-value
hash table and reverse creation root at0007bc48/0007bc40. Selection now uses
the actual candidate set; cleanup releases and clears the actual WorkValue
records before arena-two reset. The recovered24-byte layout supersedes the old
OptimizerTreeEntry names. These corrections preserve the original function
graph while fixing the shared state needed by later functions in this compile.

Fresh clean-build captures match407 normalized lines at transfer insertion
return and413 at range-recording return, including five inserted scalar loads,
all38 register bindings, all eight definition-owner slots and six variable
records. A separate nine-line capture at the next logical trap verifies both
selection-global-header pointers equal the candidate set containing32..37,
ECX identifies the selected entry block and the work-value root has ID11.
Return breakpoints stop before the caller releases definition sets; the
selection capture reads the machine-block body only after its ownership changes.
Hinted stores, pinned placement, critical-edge splitting, floating ranges and
populated cache cleanup remain outside this checkpoint's dynamic validation.
See cases/hello-register-transfers-abi.json for extents, ABI and ownership evidence.

Historical replay now uses replay-frontiers.py with four isolated original/
reconstruction pairs by default. Each frontier has separate masked binaries,
IR inputs, working directories and debugger output under the private scratch
area. Every pair must hit its recorded logical trap, and hashes assert the
native build/config stayed frozen. After all pairs finish, the ordinary final
comparator retains baseline, streams and object checks. Set REPLAY_JOBS=1 for
one pair at a time; compare.py --replay also retains the earlier serial path.

This checkpoint has447 reconstructed functions and524 accepted frontiers.
The clean build, all352 minimal and172 hello historical frontiers, all source
graphs and all accepted instruction boundaries pass. Six affected modules
compile /W3 clean; the minimal object/exit/streams comparison passes. Verified
accepted code-extent coverage is138,495 /399,693 bytes, or34.6503percent,
including source/shared text/CRT/import thunks with union, clipping and actual-
mask verification. This is not branch coverage. Hello remains pending at
prepare_machine_context,00023e30, whose next task is machine-transfer placement.

The first real machine block now completes register-transfer placement.
Thirteen more functions cover546 native instructions: transfer construction,
canonical register-home lookup, symbol/lvalue operands and address allocation,
condition selection, opcode lookup, placement search and explicit register-use
tests. prepare_machine_context now has its recovered int return, reporting
whether it moved a record. Its callers currently ignore that result. Private
allocation lookup selects either a range or its owning value through the same
fastcall2 graph vertex; ordinary C void* expresses that variant result.

Independent captures match49 normalized lines before placement and49 after it.
All16 records retain matching links, complete opcode/flag words, source markers,
header live sets, register identities, parameter memory home and branch targets.
The load moves past its source marker and receives the native800000 placement
bit. The next width query receives register4022. These captures inspect the
selected machine block after WorkItem.body changes ownership; released source
expressions and definition sets are not interpreted as live records.

The bounded intrinsic-flags table has two entries permitting intrinsic171's
zero equality/inequality comparison. Its readable predicate replaces those
semantic entries without retaining table bytes. The broader intrinsic identity
remains deferred to its selector. Floating compares reuse the existing precision
flag at00080698; their arrangement helper remains a named trap. Opcode lookup
retains the original255 stop test despite the observed maps ending in-1.
Pinned placement, later blocks, stores, positive intrinsic shortcuts and fatal
paths remain outside this checkpoint's dynamic validation. See
cases/hello-machine-placement-abi.json for exact extents, ABI evidence and limits.

This checkpoint has460 reconstructed functions and537 accepted frontiers.
All352 minimal and185 hello historical frontiers replay. All source graphs and
accepted instruction boundaries pass; seven affected modules compile /W3 clean.
The fresh minimal object/streams/exit regression passes. Verified accepted
code-extent union is140,111 /399,693 bytes, including source/shared text/CRT/
import thunks with clipping and actual-mask verification; this is not branch
coverage. Hello remains pending at register_storage_bytes,0002e0e0.

Seven more functions now carry the optimized first block through storage and
register-width analysis, redundant comparison elimination and operand identity.
RegisterVariable offset 0x28 is a signed storage width; its lookup flag requires
an existing record rather than creating one. A private replacement predicate
uses ECX plus one stack argument with EDX unused, expressed as typed fastcall C.
The full operand overlap tree retains its asymmetric absolute/indexed rules
and avoids reading beyond short frame-temporary symbols when testing private
homes. Its interval callee remains a named trap until that path is encountered.

Comparison elimination retains the earlier flag producer, unchanged-operand
checks, signed/inclusive condition relations and branch combination. Named
opcode and condition constants express the bounded native tables. All 231 ISA
implicit-register properties were independently checked before their reuse;
no copied section or opaque data was introduced. See
cases/hello-instruction-analysis-abi.json for exact extents, ABI and provenance.

At the first call-emission frontier, an independent capture matches 39 lines
of completed work1: eleven linked records, full opcode/operand flags, register
use links, memory homes, branch conditions and block header sets. The next
call target also matches _DefWindowProcA@16, symbol3940 and type4. Current
call selection still owns expressions, so the capture explicitly reads the
completed earlier block rather than interpreting the current body as machine
records. Its final boundary links onward outside the captured interval.

This checkpoint has 467 reconstructed functions and 544 accepted frontiers.
All 352 minimal and 192 hello historical pairs replay; all source graphs and
accepted extent instruction boundaries pass. Three affected modules compile
/W3 clean; minimal object/streams/exit match except the four-byte timestamp.
Accepted original code-extent union is 142,255 / 399,693 bytes (35.5911 percent),
including source, shared text, CRT and import thunks with clipping and actual
mask verification; this is not branch coverage. Physical implicit masks,
interval overlap and failure paths remain named traps and are not dynamically
claimed. Hello remains incomplete at emit_call, 000195a0.

The first optimized hello function now completes generate_work and enters
backend emission. Five more functions reconstruct call records, physical
implicit-register masks, definition widths, address stability and complete
LEA folding. Call metadata now names argument byte count, register mask and
result flags in its existing twelve-byte arena record. The implicit-register
selector retains every native opcode/mode rule, including conservative mode0
behavior and the local-label call convention. Definition widths preserve
partial EAX call returns and explicit high-byte lanes.

LEA folding combines typed base/index words, scale, symbol and displacement
without adding graph vertices. Wrapping displacement arithmetic uses unsigned
32-bit operations. The full native tree retains test/apply differences,
absolute-source symbol replacement, repeated-register rejection and a possible
base/index swap before a later failure; broader branch execution is not claimed.

Six fresh captures at sync_machine_work match271 normalized lines across all
six completed machine blocks, including92 record visits with repeated boundary
headers. Full flags, links, live sets, filename/source markers, branch targets
and all seven calls agree. Call argument sizes include4,8,16 and20; every
captured register mask is zero and result flag is1. The context's ECX work
argument matches the entry work pointer in every capture. See
cases/hello-completed-generation-abi.json for extents, ABI and capture receipts.

All352 minimal and197 hello historical frontiers replay after these changes.
All472 source graphs and549 accepted extent instruction boundaries pass; four
affected modules compile /W3 clean after an existing comparison warning was
corrected with an explicit equivalent conversion. Minimal object/streams/exit
still match except only the four-byte COFF timestamp. Accepted original code-
extent union is144463 /399693 bytes (36.1435 percent), including source/shared text/CRT/
imports with clipping and actual-mask verification; this is not branch coverage.
Hello remains incomplete at sync_machine_work,00068b70. Later functions,
physical allocation/scheduling and optimized final emission remain pending.

Seven more functions reconstruct backend label synchronization and whole-stream
finishing. The kind1 label/kind2 function-header overlay now names chain links,
optional attachments and label flags at their recovered offsets. Synchronizing
rebuilds the side chain and exact reference counts, then marks region-header
alignment. Scanning merges canonical labels across source/block markers,
preserves their alignment/attachment position and redirects branch/data label
operands with balanced references. Compaction removes unreferenced labels
while retaining protected kind4 labels. The separate cdecl wrapper and fastcall
stream helper at1dc70/1dc80 remain separate original graph vertices.

The optional attachment has only an observed24-byte prefix through its label
back-pointer. Its owner and full identity remain unresolved; no reconstruction
allocates or copies attachment objects. Positive motion/attachment branches
have full parent source, but their insert_machine_after callee is still a named
trap and these branches are not dynamically claimed. See
cases/hello-synchronization-abi.json for exact extents, ABI and layout limits.

At analyze_machine_instructions, six fresh captures match291 normalized lines
and90 record visits including repeated boundary headers. The surviving label
chain is9280,9278,9270 with signed reference counts1,1,2; all flags are zero and
attachments are null. Full machine records, operand flags, live sets, source
markers, branch identities and all call metadata continue to match.

All352 minimal and204 hello historical frontiers replay. All479 source graphs
and556 accepted extent instruction boundaries pass; three affected modules
compile /W3 clean. Minimal object/streams/exit match except only its four-byte
COFF timestamp. Verified accepted original code-extent union is145263 /399693 bytes
(36.3436 percent), including source/shared text/CRT/imports with clipping and
actual-mask verification; this is not branch coverage. Hello remains pending
at analyze_machine_instructions,00048040.

Machine analysis now builds the first function's regions. Seven functions
cover the entry wrapper, full11-way stream-kind dispatch, region factory,
reverse-order operand label lookup, open-region reference processing and
reference allocation/consumption. The builder retains native block0x200 marks,
flag barriers, residual reference counts and typed range.c diagnostics.

The new producer resolves the former MachineLabelAttachment: it is the same
96-byte MachineFlowRegion, allocated in arena2 with its label back-pointer at
0x14. One region type now serves both uses. Its three reference heads occupy
1c/20/24 and own eight-byte next/label records; field24 is no longer numeric.
The count helper also consumes matching nodes from the first two chains, and
leaves the third chain intact. Other unknown fields remain named by offset.
No attachment image, raw section bytes or duplicate region state were added.

Independent captures match298 normalized lines: seven for the root plus291
across all six work blocks. The root spans an87-record stream, from index0 to
its terminating kind3 record at85, with function-context label at3. It has no
parent/sibling/children, all three reference chains are empty, block bounds and
reference counters are zero, and flags are0x49. The context's attachment is
exactly that root, and all unresolved fields are zero. Full block/header flags,
label counts, operand state and call metadata also match. Since this function
has no child regions, the recursive analysis helper remains a named trap.
See cases/hello-machine-regions-abi.json for exact ABI, provenance and receipts.

All352 minimal and211 hello historical frontiers replay; all486 source graphs
and563 accepted extent instruction boundaries pass. The two affected modules
compile /W3 clean. Minimal object/streams/exit match except only the four-byte
COFF timestamp. Verified accepted original code-extent union is146671 /399693 bytes
(36.6959 percent), including source/shared text/CRT/imports with clipping and
actual-mask verification; this is not branch coverage. Hello remains pending
at simplify_machine_data,000358b0. Recursive/closed/special-reference paths,
child linking, floating cases and range failure paths are not dynamically
claimed; their unvisited callees remain typed named traps.

Five more functions carry the first function to physical register allocation.
The floating preparation/peephole gates reuse current_function's existing
floating bit and precision mode. The global floating peephole path calls the
same359e0 selected-block helper with null block/mode0. The immediate-register
wrapper uses the existing -noimmreg option and i486-or-later CPU gate, then
snapshots next_tree_register and calls its separate region helper even when
the list is null. The final optimization hook is an actual empty RET.

The full429-instruction immediate helper is reconstructed: recurse through
regions, collect up to sixteen eligible constants outside children/protected
blocks, select at most four frequent values, create register-backed loads and
rewrite matching immediates. Typed24-byte local candidate records are populated
from machine operands; no original data table is copied. This first invocation
has a null list, so positive selection/insertion/definition paths remain outside
dynamic validation, with their named callees still trapping.

Tracing insertion exposed another shared state slot. Original830d0 is both the
old expression transformation cursor and the machine insertion cursor. It now
has one TransformationCursor union with phase-specific expression/instruction
members; no duplicate pointer was introduced. The active machine-region pointer
830c8 is separate from root830c4. Source expression cursor assignment was
updated to its union member; populated phase transitions remain to be exercised.
See cases/hello-immediate-pass-abi.json for exact graph, ABI and ownership limits.

Fresh independent captures match299 normalized lines: eight for root/cutoff
state and291 across all six work blocks. The root/87-record stream remains
unchanged, the cutoff and next-register values are both38, and the shared cursor
is null. All352 minimal and216 hello historical frontiers replay; all491 source
graphs and568 accepted instruction boundaries pass. Both affected modules
compile /W3 clean. Minimal object/streams/exit match except only the four-byte
COFF timestamp. Verified accepted original code-extent union is148383 /399693 bytes
(37.1242 percent), including source/shared text/CRT/imports with clipping and
actual-mask verification; this is not branch coverage. Hello remains pending
at allocate_machine_registers,0002aa80.

Physical register allocation now reaches its conflict-graph builder. The whole
allocator driver is reconstructed: establish available colors, compare both
ordering strategies by signed spill cost, retry after spill rewriting, and
select EBP/ESP frame addressing before assigning machine operands. Its later
coloring/spill/frame helpers retain separate named traps; only the prelude is
claimed dynamically. Seven more complete functions cover eligibility, floating
block iteration, physical variable construction, rematerialization pruning,
allocation-round reset and the shared next-block iterator.

RegisterVariable retains its52-byte layout with typed work/conflict links,
signed degree/spill-cost shorts, priority/weight and deferred machine blocks.
Original current_work and stack_frame_extent state are reused. An independent
capture caught a preliminary mistaken alias between the eligibility gate's
stack state80688 and the unrelated -Gy switch806e4; the corrected capture
agrees on7 available colors, limit2,20 variable records,12 counters,31 cleared
physical costs,32 cleared temporary heads and exact current-work/callback
identities. It matches41 normalized lines. See
cases/hello-physical-prelude-abi.json and cases/allocation-state.txt.

The native statistics clearing spans69 DWORDs. Only12 active counters are
represented contextually;57 clear-only words are omitted after checking mapped
pointer constants, indexed neighboring arrays, pointer arithmetic and address
escape. No original data snapshot or opaque native gap is introduced. The
spill-home hash and physical cost tables have semantic types and extents from
their constructor/index consumers.

All352 minimal and223 hello frontiers replay after the correction;498 source
graphs and575 accepted extent instruction boundaries pass. Three changed
modules compile /W3 clean. Minimal object/streams/exit match except only the
four-byte COFF timestamp. Verified accepted original code-extent union is
149679/399693 bytes (37.4485 percent), with source/shared text/CRT/imports
unioned, clipped and checked against actual masks; this is not branch coverage.
Hello remains pending at build_register_conflicts,0002bbd0.

Nine more functions enter register interference construction. Each selected
block starts with a private iteration of its live-in set. Reverse operand
visits account for direct registers, address bases/indexes, last uses and the
excluded source of a coalescible MOV. Separate forward byte-operand checks
exclude ESI/EDI/available EBP. Implicit register conflicts preserve their own
helper and typed indirect callback. Arena3 owns temporary live-list nodes;
fixed frame/stack and noninteger physical registers are excluded as native.
The work's original flow-set slot is reused, without another liveness cache.

Native opcode property0x40 maps to existing MC_IMPLICIT_REGISTERS, whose231
rows were previously checked. That semantic enum translation was corrected
before final replay. The main operand helper keeps its full four-argument
fastcall ABI; signed annotations, loop depth and callback stack cleanup are
preserved. The first combined cost query is now allocation_operand_cost2bff0,
with register4022, annotation0, operand0, apply0 and both optional pointers
null. An independent capture matches41 normalized lines, including those
register/stack arguments,20 variable records and12 counters. Positive virtual
costing, populated interference and rewriting remain outside dynamic claims.
See cases/hello-register-conflicts-abi.json.

All352 minimal and232 hello historical frontiers replay;507 source graphs and
584 accepted extent instruction boundaries pass. The new module compiles /W3
clean; minimal object/streams/exit match except only its four-byte timestamp.
Verified original code-extent union151423/399693 bytes (37.8848 percent),
including clipped source/shared text/CRT/import ranges with actual-mask
verification, is not branch coverage. The queued hello case remains active.

The first function now completes its register conflict graph. Six more whole
functions include the617-instruction combined spill-cost/rewrite routine,
placeholder/actual home construction, folded memory costs, loop weighting and
symmetric edge insertion. Measurement and mutation remain in the original
fastcall7 vertex with five stack arguments. Its context is a typed WorkItem*
whose consumer uses the existing flow-set slot. Four counters now have names
from their recovered rematerialization/folding/load/store increments.

Depth weighting introduces only the four contextual values1,5,25,125 at836f8;
unsigned depth>=4 returns125+depth. Spill homes use the typed32-head temporary
hash or existing variable home. The conflict callback retains pointer-identity
deduplication, two arena4 links and short degrees. No image section or opaque
snapshot is copied. See cases/hello-spill-cost-abi.json for ABIs and limits.

An independent capture at simplify_unspillable_variables2bb50 matches104
normalized lines:24 complete variable records,14 undirected/28 directed edges,
exact canonical bucket-pointer identities, symmetric unique edges, every degree
and signed cost, all31 physical costs and12 counters. Six complete work-block
captures match291 more lines. The first function exercises cost measurement
and memory folding; apply1 spill/recreation paths remain unvisited, with their
named callees still trapping. Optimized hello output is not yet claimed.

All352 minimal and238 hello frontiers replay;513 source graphs and590 accepted
extent instruction boundaries pass. Three modules compile /W3 clean. Minimal
object/streams/exit match except only the four-byte timestamp. Verified accepted
code-extent union154143/399693 bytes (38.5653 percent), including clipped
source/shared text/CRT/import ranges checked against actual masks, is not branch
coverage. Reconstruction continues through allocation simplification.

Nine more functions collect and sort copy affinities before coalescing. The
negative-cost simplifier is reconstructed in full; this first function has no
negative final variable costs and skips its spill branch. The separate driver
collects once, then repeatedly sorts and asks the still-trapping coalescer to
merge one admissible pair. MOV/LEA weighting, representative lookup, fixed and
interfering pair rejection, canonical16-byte record construction, duplicate
benefit accumulation and descending list insertion retain their original graph.

RegisterVariable+1a is consistently named representative across constructor,
round reset, spill-home use and collection. The existing52-byte layout stays
intact. Live/recycled affinity heads81114/81118 are typed pointers; records are
computed from operands. Zero/one/larger LEA displacement magnitudes use weights
8/4/1 with unsigned magnitude arithmetic. No original data table is copied.
Current input has only one affinity4020/4022 with benefit8 and no recycler.
Duplicate/recycler/nonpositive-disposal paths remain outside dynamic claims.

Fresh independent input capture matches107 normalized lines: exact live-head
and endpoint identities, canonical ordering/benefit, all24 variables and14
undirected conflict edges. All352 minimal and247 hello frontiers replay;
522 source graphs and599 accepted extent instruction boundaries pass. Four
affected modules compile /W3 clean; minimal object/streams/exit match except
only the four-byte timestamp. Verified accepted code-extent union155087/399693
bytes (38.8015 percent), including clipped source/shared text/CRT/import ranges
and actual-mask verification, is not branch coverage. The next frontier is
coalesce_register_affinities2e0f0. See cases/hello-register-affinities-abi.json.

The first function now coalesces and colors without spills. Eight more whole
functions preserve class preference, representative chains, graph/affinity
updates, recycling, signed cost priorities, reverse removal ordering, explicit
degree recounting and greedy coloring. The native graph still separates every
constructor/list/edge helper. A three-class precedence relation prefers physical
then tree then machine registers, with larger-degree choice within a virtual
class. The seven named colors are EAX/ECX/EDX/ESI/EDI/EBX/EBP; their28-byte native
extent ends exactly before the writable affinity head. Both data recipes are
small contextual semantic tables, with no adjacent image data copied.

Two independent captures match242 normalized lines. Coalescing aliases4020 to
4022, giving signed cost20, priority1000 and weight2000, with the exact recycled
affinity identity. Successful coloring matches25 complete variable records and
canonical work-list links; all14 interference pairs have distinct colors.
Window/message/DC use ESI/ECX/EDI, strategy0 succeeds, and frame pseudo-register24
is created with colorEBP. All31 physical costs and contextual counters agree.
Hard-constraint counting, nonempty losing-edge transfers, strategy1, retries,
EBP allocation and spill rewriting remain outside dynamic claims. See
cases/hello-register-coloring-abi.json.

Analysis of the next pass refined the neighboring-state audit: the28-byte
register-state array starts7c3a0 and ends7c480. Its reset walks pointer fields
at offsets0x14/0x18; the comparison limit7c494 is not the object-array end.
The provenance note is corrected without changing accepted code masks.

All352 minimal and255 hello frontiers replay;530 source graphs and607 accepted
extent instruction boundaries pass. Both changed implementation modules compile
/W3 clean; minimal object/streams/exit match except only the four-byte timestamp.
Verified accepted code-extent union156399/399693 bytes (39.1298 percent), including
clipped source/shared text/CRT/import ranges and actual-mask verification, is not
branch coverage. The next frontier is optimize_allocated_machine_regions2b6a0.


Physical register-value reuse checkpoint (616 accepted frontiers)
---------------------------------------------------------------
Nine whole functions preserve the region/block walk, definition/last-use alias
lifetimes,383-instruction value-reuse pass and deep operand copy. The block
rewriter includes its11-entry dispatch table and retains memory splitting,
move insertion and cloning as calls to named children. The native graph and
fastcall/cdecl ABIs remain intact. An eight-entry28-byte cache holds named
operation/saved-operand/reference fields; block resets clear only reference
pointers. The neighboring eight-WORD permutation, two color-history markers,
region pointer, live masks and eight alias heads are distinct typed objects.
The contextual8-byte alias node allocates in arena3. No surrounding storage is
copied. Address-ownership and memory-splitting predicates use encountered
semantic kind/opcode sets rather than normalized property bits from another
table. In particular operand-copy address ownership includes kinds3..7 and12.

A fresh independent capture matches191 normalized lines, including87 complete
instruction records, linked-use/previous identities, aliases EAX:4023 and
ECX:4022, live/prior masks13/12 and the EAX cache's owned lparam operand.
Both reach register_reuse_barriers at2f3f0 before any physical-color replacement;
register preference construction, substitutions, inserted moves and failure
branches are retained but not claimed dynamically validated. See
cases/hello-register-rewrite-abi.json for exact extents, ABI/data provenance,
capture hashes and frozen replay receipts. The optimized hello object remains
unfinished; no success equivalence is claimed for it.

All352 minimal and264 hello frontiers replay,539 source graphs and616 accepted
instruction boundaries pass, and physical-rewrite compiles /W3 clean. Minimal
object/streams/exit still match the unmodified original and fixture except only
COFF timestamp bytes4..7. Verified accepted original code-extent union is
160111/399693 bytes (40.0585 percent), including clipped source/shared text/CRT/
import ranges and actual-mask verification; this is not branch coverage.


Completed first-function register-value reuse (621 accepted frontiers)
--------------------------------------------------------------------
Five whole helpers recover interval write barriers, preferred MOV/LEA colors,
cache equivalence after canonical coloring, preference order and independent
last-use tests. Register27 remains an absent base,26 a missing preference;
no lookup or cache index conflates either with an integer color. Both three-
argument helpers preserve ECX/EDX plus one RET4 stack slot. The cached-operand
switch includes its encountered32-byte shared text within the function extent.

The first function now completes value reuse. An independent capture matches
188 normalized lines and87 instruction records, including an actual linked
lparam definition/use replacement from logical4023 to EDX in records26/27.
All eight alias buckets retire, live mask becomes ESP-only, and both register-
history markers reset to26. No memory-split moves are inserted in this case.
The permutation is built contextually; unused entries retain their prior values.
Spill/memory splitting and alternate failure branches remain outside dynamic
claims. The next matching trap is assign_machine_registers2b330, whose full
analysis identifies save/frame-setup pruning rather than operand assignment.
See cases/hello-register-reuse-order-abi.json for ABI and capture/replay receipts.

All352 minimal and269 hello frontiers replay;544 source graphs and621 accepted
instruction boundaries pass. physical-rewrite compiles /W3 clean. Minimal
object/streams/exit match unmodified original and fixture except only timestamp.
Verified accepted original code-extent union163119/399693 bytes (40.8111 percent)
includes clipped source/shared text/CRT/imports and actual-mask cross-check;
it is not branch coverage. Optimized hello remains incomplete.


Frame-symbol assignment and save pruning (628 accepted frontiers)
---------------------------------------------------------------
Seven whole functions recover written-color/save/frame-setup pruning, the386
frame-reference cost estimate, repacking of referenced locals, its weighted
size-group list and scope symbol visiting/assignment callback. The stateful
symbol iterator preserves selected-scope/previous-node identity and its64
hash buckets. Existing save masks and frame counters are reused at their
canonical definitions. The encountered list node is typed16-byte next/size/
weight/symbol data, allocated in arena3. Only its live local-head pointer is
represented: native analysis proves the adjacent cleared stack word is never
read and its address never escapes. See cases/hello-frame-symbols-abi.json.

Independent frame capture matches15 normalized lines and12 symbols. The new
paint/client locals are64/16 bytes at -64/-80 with exact assignment flags;
register-resident dc retains its old offset without a new frame assignment.
All parameter offsets remain8/12/16/20, local offset is -80, next parameter24,
and canonical frame extent remains0 until ESP conversion. A second capture
matches184 normalized lines/85 instruction records: the unused EBX save and
restore are removed, while ESI/EDI saves remain. No raw address is treated as
a frame offset for non-variable symbols. The next matching trap is
rewrite_stack_pointer_frame2adf0. Optimized hello remains incomplete.

All352 minimal and276 hello frontiers replay,551 source graphs and628 accepted
instruction boundaries pass. All four changed implementation modules compile
/W3 clean. Minimal object/streams/exit still match unmodified original and
fixture except only timestamp bytes4..7. Verified accepted original code-extent
union165103/399693 bytes (41.3075 percent) includes clipped source/shared text/
CRT/import ranges and actual-mask verification; it is not branch coverage.


Completed first-function allocation (633 accepted frontiers)
----------------------------------------------------------
Five whole functions recover ESP-relative frame conversion, final operand and
block-summary physical coloring, the assignment stream walk and carry-flag
consumer search. The ESP rewriter's actual ABI returns int1 in EAX, despite
its caller discarding it; its provisional void declaration is corrected.
Its ten dispatch entries are retained within the function's code extent.
The two three-argument operand helpers preserve fastcall ECX/EDX and RET4.
Only valid virtual/frame colors are resolved, physical analysis flag WORDs
are cleared, and byte values require an encodable byte-register color.
Block bitsets gain physical colors while retaining logical members; a copied
set isolates iterator state during updates. Carry search distinguishes CF
conditions2/3/6/7 from ZF/AF-only users and stops at redefinitions/transfers.
No new data blobs or assembly are introduced.

Independent ESP and final assignment captures match181/208 normalized lines,
including83 instruction records, exact operand/link identities and complete
block bitsets. The first function's allocation now returns: a fresh post-
cleanup capture matches210 lines and82 records, with all explicit registers
physical. The next matching trap is prepare_machine_blocks2e700. Subsequent
copy propagation, peepholes, scheduling/lowering and remaining functions keep
optimized hello incomplete. See cases/hello-ESP-assignment-abi.json and
cases/hello-allocation-complete-abi.json; unvisited/failure branches are not
claimed dynamically validated.

All352 minimal and281 hello frontiers replay,556 source graphs and633 accepted
instruction boundaries pass. physical-allocation, frame-symbols and
instruction-uses compile /W3 clean. Minimal object/streams/exit still match
unmodified original and fixture except only timestamp bytes4..7. Verified
accepted original code-extent union166943/399693 bytes (41.7678 percent)
includes clipped source/shared text/CRT/import ranges and actual-mask
verification; this is not branch coverage.


Post-allocation physical copy propagation (638 accepted frontiers)
----------------------------------------------------------------
Five whole functions recover the145-instruction walk,393-instruction MOV/LEA
elimination pass,194-instruction operand propagation and separate eight-byte
copy constructor/recycler. Thirty-one named register-copy heads7c634..7c6b0
end at a separate recycler word and remain disjoint from statistics/costs.
Nodes hold a typed instruction pointer and next link at offsets0/4, allocated
in arena3. Existing counters7c544/548 are now named eliminated_copies and
propagated_copies from all their mapped updates, retaining canonical layout.
No original image data is copied. Native x87 property1 uses the already
audited opcode ranges, rather than a new raw flags table.

The complete function graphs preserve memory-change, byte-lane, overlapping
address, ESP-relative and floating-copy alternatives. Native22740 is the
mutating replace_machine_operand with apply3; graph audit corrected an
initial sibling-predicate selection before paired acceptance. The three-
argument propagation helper retains fastcall ECX/EDX and RET4. Stack-relative
and floating alternatives keep named trapping children and are outside
current dynamic claims.

An independent constructor-return breakpoint matches243 normalized lines,
including the ECX copy bucket owning instruction11, all31 heads, empty
recycler, counters and disjoint/acyclic ownership. This observes live nodes
before arena retirement. The completed first-function pass matches210 lines/
82 stream records with exact physical operands, links and block bitsets.
The next matching frontier is set_machine_frame351a0; further post-allocation
passes and the remaining function keep optimized hello incomplete.
See cases/hello-physical-copies-abi.json.

All352 minimal and286 hello frontiers replay,561 source graphs and638 accepted
instruction boundaries pass. post-allocation and physical-allocation compile
/W3 clean. Minimal object/streams/exit remain exact except only timestamp
bytes4..7. Verified accepted original code-extent union169375/399693 bytes
(42.3763 percent) includes clipped source/shared text/CRT/import ranges and
actual-mask cross-check; it is not branch coverage.


Frame-access optimization and stable sorting (646 accepted frontiers)
-------------------------------------------------------------------
Eight whole functions recover frame-use/definition collection, its typed
16-byte arena4 constructor, stable bottom-up list sorting and merge callback,
frame-byte displacement comparison and grouped dead-store elimination.
The sorter uses only the first-field next pointer; comparison preserves
native callback order and puts address-taking LEAs before reads of the same
byte. Symbol placement, changing ESP depth and high-byte lanes contribute to
the byte comparison. Indexed references retain their whole-symbol meaning.

Arena4 clears its payload before allocation, so retained flag8 and padding
start zero; selector1/2 and protected4 are set by the constructor. The combiner
retains protected/overlapping writes and the original graph, including the
existing side-effect predicate23670. Its operand-byte call deliberately keeps
the native one-based selector. No raw image data or new assembly is introduced.

An independent sorted-list capture matches239 normalized lines:9 reads and
no definitions, exact selected operands/symbol offsets/stack depths/flags,
complete82-record stream and disjoint acyclic node ownership. The first
function's frame pass returns; a later capture matches193 lines/82 records
after existing context finalization. Definition deletion and retained/protected
alternatives remain dynamically unexercised in this first function. The next
matching frontier is peephole_machine_33f50 at33f50; hello remains incomplete.
See cases/hello-frame-accesses-abi.json for ABI and capture/replay evidence.

All352 minimal and294 hello frontiers replay,569 source graphs and646 accepted
instruction boundaries pass. frame-access and linked-lists compile /W3 clean.
Minimal object/streams/exit match except only timestamp4..7. Verified accepted
original code-extent union171743/399693 bytes (42.9687 percent) includes clipped
source/shared text/CRT/import ranges and actual-mask verification; it is not
branch coverage.


Comparison sharing across branches (652 accepted frontiers)
----------------------------------------------------------
Six whole functions recover the302-instruction cross-branch comparison pass,
generic machine-record equality, preceding-block protection, next-operation
scanning, defining-label lookup and reversed-operand matching. The pass now
has the meaningful name optimize_branch_comparisons. Its entire graph retains
Pentium load/register-renaming and clone/retarget alternatives as named calls.
The generic comparator handles record kinds, operand counts and string/repeat
prefixes using existing semantic properties, with its12-entry kind dispatch
inside the original extent. The reversed matcher preserves high-byte lanes
and owns a contextual16-entry exchange map expressed with x86 condition names.
No data section snapshots or assembly are introduced.

The first-function pass returns; an independent completed-stream capture
matches193 normalized lines/82 records with exact operands, links and block
bitsets. Successful retargeting/register-cloning, string-prefix recursion and
failure paths are not claimed exercised. Remaining children keep named traps.
The next matching frontier is peephole_machine_33550 at33550; optimized hello
remains incomplete. See cases/hello-comparison-branches-abi.json.

All352 minimal and300 hello frontiers replay,575 source graphs and652 accepted
instruction boundaries pass. All three changed implementation modules compile
/W3 clean. Minimal object/streams/exit match except only timestamp4..7.
Verified accepted original code-extent union173647/399693 bytes (43.4451 percent)
includes clipped source/shared text/CRT/imports and actual-mask cross-check;
this is not branch coverage.


Unconditional jump threading (660 accepted frontiers)
----------------------------------------------------
Eight whole functions recover the235-instruction jump pass, embedded DATA
range movement, function-end/terminator searches, preceding-operation scan,
label predecessor reachability, signed imm8 predicate and generic record
cloning. The pass now has the meaningful name optimize_unconditional_jumps;
its full17-callee graph retains range movement, short-tail duplication and
branch-diamond alternatives. The old provisional label_has_backward_branch
name is corrected to label_has_fallthrough_predecessor after full native
analysis of RET/JMP/DATA and preceding labels. All source users are updated.

Record clones preserve header/link identities before linking, then own fresh
typed address payloads and copied block bitsets where the native record kind
requires them. The first live clone-return capture matches196 normalized
lines: source instruction75 is POP EDI, detached and distinct from its source,
with exact links and payload. This first clone has no address data, so deep
address/block-set alternatives are not claimed exercised. The completed
first-function jump pass matches207 lines/88 stream records, six more than
before tail duplication. Embedded DATA moves, diamond merging, fresh-label
loop avoidance and failure paths retain their graph but remain outside these
dynamic claims. No image snapshots or assembly are introduced.

All352 minimal and308 hello frontiers replay,583 source graphs and660 accepted
instruction boundaries pass. All four changed implementation modules compile
/W3 clean. Minimal object/streams/exit remain exact except only timestamp4..7.
Verified accepted original code-extent union175167/399693 bytes (43.8254 percent)
includes clipped source/shared text/CRT/imports and actual-mask cross-check;
this is not branch coverage. Next matching frontier is peephole_machine_34520
at34520. Optimized hello remains incomplete; see cases/hello-jump-threading-abi.json.


Late branch factoring and adjacent peepholes (663 accepted frontiers)
-------------------------------------------------------------------
Three whole functions recover321-instruction common branch-code factoring,
74-instruction unreachable cleanup and227-instruction adjacent-operation
peepholes. Their meaningful names describe shared tails/prefixes, deletion
between transfers and reachable labels, and CALL/RET/arithmetic/stack restore
simplification. The complete native graphs and inline label-chain updates are
preserved. Existing canonical frame-size and dynamic-stack state are reused;
no new globals, image tables or assembly are introduced. Fresh-label factoring
and LEAVE expansion keep named trapping children and are not claimed exercised.

The first function now reaches instruction scheduling. An independent entry
capture matches206 normalized lines/87 records with exact operands, links and
block sets, after the three passes and existing analysis. This verifies their
combined live result; record-count changes are not assigned to unobserved
individual branches. See cases/hello-late-peepholes-abi.json. Optimized hello
remains incomplete at schedule_machine_instructions3f240.

All352 minimal and311 hello frontiers replay,586 source graphs and663 accepted
instruction boundaries pass. jump-threading and late-peepholes compile /W3
clean. Minimal object/streams/exit match except only timestamp4..7. Verified
accepted original code-extent union177183/399693 bytes (44.3298 percent) includes
clipped source/shared text/CRT/imports and actual-mask verification; it is not
branch coverage.


Scheduler entry and typed state (666 accepted frontiers)
-------------------------------------------------------
Three whole functions recover the386-instruction scheduling driver,63-
instruction memory-run cost predicate and checked allocator. The driver keeps
all13 native callees, complete window/barrier/protected-block/Pentium/region
and cleanup control flow. Scheduler state is defined contextually as named
scalars, a typed12-byte arena4 region frame and an eight-element x87 value
array. Native7c2e0+architectural-register14..21 aliases that same physical-array
storage; consumers translate the base/index pair to reg-REG_ST0. Value serial,
positions pointer and heap ownership are separate live fields, all retained.
No image section snapshot or assembly is introduced.

The register-tracking table is recovered as SchedulerRegisterState**, after
its next consumer41df0 identifies each entry as a pointer to per-register
state. It is not a counter table. Independent initial-state capture matches
217 normalized lines/87 records: capacity65, all65 pointers NULL, ordinal0,
memory-run2, x87 depth0/serial1, eight values -1, and empty region/positions
ownership. The whole driver is reconstructed, but only its initialization,
prologue scan and allocation have executed so far. Instruction information,
windows, post-folding and floating alternatives keep named trapping children.
Next matching frontier is prepare_scheduled_instruction3fa40; optimized hello
remains incomplete. See cases/hello-scheduler-entry-abi.json.

All352 minimal and314 hello frontiers replay,589 source graphs and666 accepted
instruction boundaries pass. scheduler compiles /W3 clean. Minimal object/
streams/exit remain exact except only timestamp4..7. Verified accepted original
code-extent union178927/399693 bytes (44.7661 percent) includes clipped source/
shared text/CRT/imports and actual-mask cross-check; it is not branch coverage.


Per-instruction scheduling information (668 accepted frontiers)
-------------------------------------------------------------
Two whole functions recover62-instruction scheduling-info preparation and
its12-instruction arena3 constructor. The existing instruction payload union
now has a typed scheduling pointer; the32-byte instruction ABI is unchanged.
The metadata record has44 bytes with ordering fields at32/36 and flags at40,
plus named x87 fields. Three dependency fields retain offset names pending
their readers. Explicit member clearing preserves the original arena_alloc
callee graph without adding memset or copying image data. The constructor's
extent includes RET41de0 and ends at the actual next entry41df0.

Independent constructor-return capture matches207 normalized lines and all
44 zero bytes before attachment. A separate first-window entry capture matches
214 lines/87 records, exact ECX/EDX stream indices11/13, three unique metadata
objects with ordinals0/1/2 and priorities0/-1/-2, and exact zero flags/x87/
dependency fields. Floating preparation and call-result classification retain
named trapping children and are not claimed exercised. The current matching
frontier is schedule_window3f840; the first window and complete driver have not
returned. Optimized hello remains incomplete. See cases/hello-schedule-info-abi.json.

All352 minimal and316 hello frontiers replay,591 source graphs and668 accepted
instruction boundaries pass. scheduler compiles /W3 clean. Minimal object/
streams/exit match except only timestamp4..7. Verified accepted original code-
extent union179183/399693 bytes (44.8302 percent) includes clipped source/shared
text/CRT/imports and actual-mask verification; it is not branch coverage.
