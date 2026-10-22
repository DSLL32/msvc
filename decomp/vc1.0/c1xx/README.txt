C1XX3232.EXE, Microsoft C++ frontend, MSVC 1.0 NT / project vc1.0

Target: toolchains/1.0-nt/bin/C1XX3232.EXE
SHA256: 333d015dd85773df2f0f9ab541ca65c40da9a2ad9bc8df9e03815f7deb20b2f7
Original image base 0x10000; entry point VA 0x804e0.
Starting case: original CL.EXE /Od /c cpp-basic.cpp. The independently verified
frontend input/output oracle is ../fixtures/cpp-basic/manifest.json.

PARTIAL reconstruction: 225 accepted paired frontiers, including 150 frontend
functions and six older CRT routines. The complete yacc driver and its 285
explicit semantic blocks are reconstructed, preserving all 186 original call
targets. The token driver and replay wrapper are also reconstructed; the next
matching trap is _enter_class_scope@8 at original VA 0x33420.
All 225 frontiers replay successfully, including exact pending FILE buffers;
the expression stream begins with the six bytes 5b8034030000. Semantic branches
will be exercised as their named callees are reconstructed. The complete
frontend case is not yet reconstructed or claimed equivalent.

Build and check the current frontier using the original toolchain:

    make -j2
    make -j2 check

make replay reruns every accepted frontier in order. compare.py --runtime
advances only exact original CRT/import matches, stopping at the next native
frontier. functions.json records the accepted order and original ranges.
identify-runtime.py matches library bytes while checking relocated direct-call
and jump destinations, so a similarly shaped but unrelated CRT routine is not
silently substituted. Driver call targets are audited by audit-graph.py.

Both on-disk original and reconstructed .text copies are masked with INT3.
Only accepted whole-function extents and explicitly recorded shared text data
are restored. The original toolchain files are never modified. The pair runs
concurrently with identical MSC_CMD_FLAGS, isolated working directories, closed
stdin, captured output and a debugger that terminates on the next trap. At
matching frontiers, visible file sets, contents and pending FILE buffer bytes are compared without
flushing or altering the child. At successful
completion, streams, statuses and all four intermediate files must also match
the independent original fixture exactly. Scratch and a private 32-bit Wine
prefix live under ~/.ai-tmp/vc1.0-c1xx.

The only inline assembly currently used in native source is INT3 in named
unreconstructed stubs, as required by the reconstruction protocol. Native
implementations use readable C; original CRT code is linked from the original
library after exact matching. No executable data sections are copied into C.

The original CRT __fmode is 0x8000 at VA 0xaa3bc. BINMODE.OBJ supplies that
same binary-default setting to this reconstruction; compare.py checks the
original and rebuilt globals against the recorded value. The PCH configuration
snapshot is represented by the typed CompilerConfiguration structure, with an
asserted size of 764 bytes. Its two path buffers and known flags are named;
remaining fields retain offsets until their consumers are reconstructed.

The launcher preserves the captured argv[0] identically for both images,
independently of the selected application image. audit-data.py checks all 98
option rules, their target sharing and defaults, the typed configuration, 81
language keywords, four __declspec entries and all 191 IR record schemas against the original binary.
The older CRT _sopen, setvbuf, _freebuf, fseek, __tzset and _isindst bodies have readable C replacements;
audit-runtime-variants.py and runtime-variants.json record their complete code
and call/data alias correspondence with the shared older C1 variants. Assembly
is used only as analysis input to these audits, never as generated implementation.

The current full ordered replay is recorded in evidence/replay-tag-symbols.log.
The linkage stack records the extern-C/C++ language mode; it is separate from
the symbol scope chain. C++ type descriptors are twelve bytes and include a
signed short in their identity hash. Function signatures additionally intern
default arguments and an associated type, rather than only declaration types.

The special-identifier initialization table is audited by all 41 spellings,
initial targets and observable target sharing. Character properties are a
semantic classification table, checked against all 256 original entries.
The older timezone parser keeps PST/PDT defaults when TZ is absent; those
names, timezone/daylight scalar defaults and the month offsets are verified
against the original and the linked CRT. Timestamp workspaces include the
observed BSS terminator after the 25 copied ctime bytes; no timestamps or
frontend output bytes are normalized by the reconstruction oracle.

The global writer retains all 37 field branches and its original 20 call targets.
Its 19 schemas and 16 shared-state defaults are checked against the original.
Function complexity aliases the expression record count, and call/inline flags
are shared by both writers. Expression call operands are typed IrToken nodes;
symbol-index operands and the current-function symbol use CppSymbol. The
initial pending global-header bytes match as 1186032e1d30016400000000.
This header contains the initial symbol count and is rewritten at completion;
it is not assumed to be a prefix of the completed fixture.

SourceFile models the 52-byte input-stack entry, with guarded overlapping input
windows and include-parent seek logic. Fifteen input defaults are checked,
including the initial UINT_MAX conditional-directive depth. Operator initialization
uses 57 contextual builtin candidates and 213 scanner token-spelling slots;
both tables are audited against their original records. A declaration retains
its storage bytes, mutable type descriptor, source-order link and reverse
circular parameter link. Its transient qualified-name link has a separate
encountered prefix. ClassInfo likewise records only the encountered prefix.

The declaration-mode cache and IR stream selector share one original byte and
one reconstructed object. State audits compare address aliases byte by byte as
well as initial values, catching equal-valued objects that would split original
state. The declaration normalizer retains all storage, class-member, linkage,
qualification and diagnostic branches, with named traps for its unreconstructed
callees. Compiler diagnostics retain the original variadic arguments.

Function signatures now intern their circular parameter-declaration types, with
the original count-ordered bucket chain and C++ hash (including the signed
class-context index). The function-shape builder includes implicit class types
and pending function qualifiers. Final declaration checks retain the original
qualified-name, browser, object/function and diagnostic branches. The shared
0x8ccb8 flag is global_expression_scope_active, rather than a /V-only state.
The SourceFile saved depth is conditional-directive nesting: the C++ directive
push routine at 0x23600 increments it and checks the 32-level limit.

Declaration resolution now retains insertion/lookup, conversion-operator storage
and qualified/static-member handling. Type folding preserves the C++ shape
categories, class distance inheritance, detached prefix chains and function/
array finalization calls. ShapeCursor is an eight-byte aggregate, including its
original by-value calling conventions. Data/code pointer-kind names now follow
their proven original globals (0x90f28/0x90f30); the rename preserves data flow.

Function-shape reconciliation includes class returns, entry-point calling
conventions and variadic member functions. Scope lookup uses interned-name
identity and retains the original temporary wrappers. Declaration reconciliation
keeps const-object, extern, typedef, alias and overload branches. Overload sets
have a separate 16-byte layout; a full declaration is 52 bytes. Default records
retain separate declaration-order and following-parameter links. The per-scope
qc_context and its shared current value are explicit; the latter's four initial
bytes are audited. All unvisited callees remain named trapping stubs.

The complete overloaded-operator validator includes member/arity restrictions,
unary/postfix conversion, allocation/deallocation types and exact diagnostic
arguments. Its three original switch tables are included in its accepted range;
the source uses readable C switches. Runtime vector helpers retain their callback
types and five-slot declaration ABI, including its unused fourth argument.
The one runtime-function specification belongs to _purecall setup and is audited
by name and attributes. The ordinary-symbol allocator preserves permanent vs
arena allocation and the separate scope-mode byte. Twenty-one option targets,
their aliases and initial defaults are audited; this established the original
normalized_Gs=1 and normalized_Gn=1 defaults before frontend initialization.
The parser uses generated logical grammar tables; no original packed parser
tables have been imported as source.

The parser's declarative grammar is recovered in grammar.bnf. grammar.py reads
only that named grammar, precedence and recovery annotations; it never reads an
original binary. Sorted LR kernels and queued closure regenerate the original
state numbering. LALR lookaheads, ordinary operator precedence and six comma
boundary overrides regenerate its actions and default reductions. Recovery
inserts identify production/dot contexts; destination states follow from shifts.
audit-parser-grammar.py compares all 147775 token decisions, 82800 gotos, lexer
query timing and 60 recovery inserts against the original. All match. The
analysis-only recovery proof is in analysis/recover-grammar.py; it is not part
of the build. The whole semantic switch is readable C in parser-*-actions.inc;
the complete C++ case remains pending.

The yacc stacks share their boundary storage with macro_work_midpoint. The
220-state array admits one extra halfword that aliases the first semantic
value; the 220-value array admits one extra eight-byte cell that aliases the
macro pointer and following control word. These overlaps use one C union.
The stack and six parser defaults are audited; all 207 frontiers replay with
these aliases in evidence/replay-parser-state.log.

generate-parser.py emits logical C tables from that same declarative grammar,
without reading the executable. The original toolchain compiles these tables
as part of the build and the accepted parser uses those logical tables.
The audit independently verifies every emitted row against the original,
including shifts after precedence resolution. All 207 existing frontiers replay
with the additional data in evidence/replay-grammar-c.log. The subsequent
whole-parser replay is evidence/replay-yacc-parser.log.

Qualified-name resolution owns its current binding, whose next link denotes
the enclosing qualifier. Declaration checks access the current target directly;
implicit-member checks deliberately inspect the enclosing link. Original loads
at 0x52031 and 0x62a1d establish that distinction. These previously unvisited
branches now use the appropriate typed view rather than an extra indirection.
All accepted frontiers replay in evidence/replay-qualification-links.log.

The accepted yacc extent is 0x268f0..0x2a524: code ends at 0x29fa1,
followed by alignment and 352 switch destinations at 0x29fa4. Its C semantic
switch retains all 285 explicit action blocks for 356 productions. Unrecovered
callees are named traps with individually recovered stack slots, including the
unused second enumerator argument, initializer argument and fourth segment
argument that the decompiler omitted. ParserValue returns in EAX/EDX with the
original eight-byte aggregate ABI under the original toolchain.

Declaration flags[-1] overlap the high declaration-depth byte; both views use
one union. Declaration specifiers and default arguments retain their preceding
cells. Forty-nine contextual declaration-code meanings are audited separately
from the unrelated expression entries in the original shared token vocabulary.
Twenty-two additional state objects are audited, including the original
text_work_cursor=expression_storage and enumerator_value=UINT_MAX defaults.
The parser's qc-tracking condition uses existing normalized_Ol; a separate
provisional object was rejected by the address-alias audit and removed.

The token driver preserves all sixteen original call targets and its complete
switch table. The replay wrapper retains the distinct forward/deferred and
pushback paths: only forward replay restores the type-specifier symbol.
Two hundred deferred records and three pushback records share one structure,
preserving their original boundary; all eight replay-state defaults are checked.
The state alias audit now includes option-rule targets and all 764 configuration
bytes. Deferred-argument text mode uses existing /E storage; prototype generation
uses existing /Zg storage (prototypes_only), rather than a separate parser flag.

The whole raw scanner preserves all 34 direct call targets, its 46 class
branches and both FILE/paged-stack /E destinations. Its punctuation paths
include member pointers, scope operators and re-dispatch after trigraphs or
splices. The 213 lexical-to-grammar categories are contextual semantic data,
checked against the original. The line driver preserves ten call targets and
its 12-way dispatch; source identifier interning retains move-to-front buckets
and its copying fallback as a named trap.

The literal/number workspace occupies 132 bytes, with the FILE pointer at its
128-byte boundary. One union owns both views. The provisional current_function
object at 0x8ccd0 is now correctly pch_mode, as proven by root-input capture
finalization at 0x6b9b0. It is distinct from both error_count (0x8cc88) and the
actual function_symbol (0x8c140). These defaults and aliases are audited.
The former adjust_type_specifier stub at 0x1e7a0 has been renamed
flush_scanned_declaration to reflect its scanner/PCH text role; its original
four-byte argument remains unchanged.

The whole type-specifier driver retains 21 call targets; its tag reader retains
16, and tag-definition setup retains 15. The source includes unvisited storage,
qualifier, constructor-name recovery, enum, inheritance-model and browser paths.
The storage-specifier child retains its unused fourth argument (RET 16), which
the decompiler omitted. Tag setup holds a completed temporary type in the active
class context; it does not prematurely replace the tag's incomplete type.
Token pushback rewinds deferred replay or stores a complete lookahead record.

The scanner EOF browser state is the existing browser_filename pointer at
0x8c404. Its earlier provisional separate flag was removed when tag consumers
proved the alias. That canonical pointer, four scope pointers, the current enum
symbol and vtordisp state are now included in the byte-address/default audits.
All 217 frontiers replay with this correction and exact pending output buffers.

Tag allocation retains its sixteen direct/indirect call targets and caller
insertion callback. A tag record is forty bytes; full declarations are fifty-two.
Both share the same encountered common fields, while tags own a separate sixty-
byte ClassInfo allocation. Compile-time size checks and original allocator
immediates establish these layouts. Registration reserves context index zero
for the sentinel and retains vector growth and exhaustion checks.

Current-scope lookup/insertion have their own wrappers, distinct from wrappers
that skip temporary scopes. The former provisional finalize_parameter_declaration
name at 0x549c0 is now insert_current_scope_symbol, since tags use it too.
The inheritance-model checker retains its contextual diagnostic names and
arguments. Class/struct compatibility warnings retain both keyword arguments
omitted by decompilation and return before consuming model flags.
All 225 frontiers replay through these changes, with no frontend normalization.
