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
