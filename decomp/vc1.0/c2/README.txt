C23232.EXE 1.0-nt reconstruction (project vc1.0)

Status: in progress. The captured minimal-C backend case currently stops at
coff_begin_function, original VA 0006cac0. Object generation is incomplete. No completed C2 output-equivalence claim is made.

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
    make frontier     assert the current logical trap and graph audit
    make replay       rerun every recorded accepted trap frontier
    make check        require complete exit 0 and exact object output

make check intentionally fails until the case reaches successful completion.
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
