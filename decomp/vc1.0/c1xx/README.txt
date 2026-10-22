C1XX3232.EXE, Microsoft C++ frontend, MSVC 1.0 NT / project vc1.0

Target: toolchains/1.0-nt/bin/C1XX3232.EXE
SHA256: 333d015dd85773df2f0f9ab541ca65c40da9a2ad9bc8df9e03815f7deb20b2f7
Original image base 0x10000; entry point VA 0x804e0.
Starting case: original CL.EXE /Od /c cpp-basic.cpp. The independently verified
frontend input/output oracle is ../fixtures/cpp-basic/manifest.json.

PARTIAL reconstruction: 342 accepted paired frontiers, including 267 frontend
functions and six older CRT routines. The complete yacc driver and its 285
explicit semantic blocks are reconstructed, preserving all 186 original call
targets. Type-symbol application, type combinations and class-body initialization
are reconstructed; class-member alignment, insertion and access labels are
accepted. Scoped C++ lookup and declaration capture are also reconstructed.
The declaration-lookahead parser is reconstructed as well; the next matching
trap is _declare_default_constructor_closures@4 at original VA 0x37070. The latest ordered replay is recorded in validation.json;
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

The current full ordered replay is recorded in validation.json.
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

Class-scope entry preserves six call targets, deferred-token suppression,
qualified wrappers, lazy bucket initialization and mode-specific allocation.
A frame is twenty bytes with one separately allocated tag-pointer cell; mode -3
allocates both permanently, while other modes use arena one. The scope-kind
predicate, stack push and directory registration are reconstructed separately.
Directory registration retains its unsigned byte index at 0xa0d1c and 128-slot
limit; it is distinct from the declaration-scope counter. Its initial value
joins the address/default audit. All 229 ordered frontiers replay exactly.

Type-symbol application keeps the original qualified name for diagnostics while
using its resolved tag/type. Type combinations retain ordering-sensitive
long/float warnings and all sign, width and aggregate recovery branches.
Class-body initialization retains its four separate helpers. Base layout saves
the next member before placement and shares the preceding-base pointer across
both passes; the low six alignment bits are distinct from its pass-selection
flag. BaseClassLink currently represents its encountered prefix, not a claim
about the producer's full allocation size. Pointer attributes come from the
first visible base when unspecified, with distinct mismatch errors/warnings.
Inheritance-model checks retain the class and both model-name arguments which
the decompiler omitted. The diagnostic table is shared where the original uses
it. Existing state 0x9f460 is #pragma vtordisp mode, established from startup,
pragma dispatch and its directive-name table; its provisional name
builtin_type_context was removed, and its zero initial value is audited.
All 236 ordered frontiers replay exactly through class-body initialization.

Arena release returns nonempty used chains to the compiler free list and leaves
its fifth pool word intact. The original diagnostic arena is the excluded root
bound. The 128-byte compiler arena and two separate release words are audited;
the intervening word between the reset globals is not merged into that state.
Type completion preserves its null-input canonical-int path and nonnull stack
consumption; sign warnings use the existing lexical spelling pointers.
Type-parenthesis resolution retains deferred capture and expression paths.
Typename recognition keeps qualified wrappers, base references and browser
reporting, with seven exact callees. The copied-qualified-declarator child has
a pointer return in EAX despite the decompiler's void label.

The whole qualified-identifier resolver retains twenty call targets, global
lookup, constructors/destructors, operator names and all recovery paths. Its
qualified-resolution head/tail object is eight bytes; each reference node has
a distinct enclosing and forward link. The original 0x914bc last-lookup-scope
pointer remains separate from the other scope pointers and joins the alias
checks. The former lookup_conversion_qualification stub is now named
lookup_qualified_member because ordinary qualified names use it as well.
Original Z rules {'a',5,target} and {'e',1,target} clear/set the same flag, so
its provisional z_a field is now extensions_enabled. The already byte-verified
rules and all users retain their behavior. Declaration-context setup consumes
a full optional descriptor, including its type index, rather than only three
storage bytes. Whole-function argument slots and constructor diagnostics are
preserved. This remains a partial reconstruction of the initial case.
All 244 ordered frontiers replay exactly with these changes.

Class-member declaration retains all 32 direct callees and its friend, anonymous-
aggregate, pure-specifier, DLL-storage, bit-field, overload and recovery paths.
Its four-byte StorageSpec arguments retain their individual stack slots. The
member-function child's apparent decompiler this pointer is dead upper-ECX
state, not an additional argument; its actual three stack slots and RET 12 are
preserved. Existing data/bit-field storage class tests are now accurately named
is_static_data_member instead of the provisional is_static_qualified_symbol.
Class list lookup keeps interned name-flag and identity checks and skips bases.

The former parse_declarator_initializer at 0x34ea0 is actually inline-member
capture, now capture_inline_member_declaration. Its complete body retains 22
call targets, paged text capture, default-argument declarations, source line
handling, synthetic special-member text and grammar recovery. All encountered
format strings are ordinary literals at their generating branches. Its output
FILE pointer and temporary /E state use the existing shared lexer storage.

Type qualifiers preserve arrays/functions and modifier behavior. Data-member
layout retains nine callees, struct/union size differences, bit-field flushing,
packing, zero-size extension recovery, overflow and aggregate property copies.
The class context now names current size, bit-field type and consumed bits;
ClassInfo distinguishes complete-object size at +0 and nonvirtual size at +4.
The common symbol auxiliary word names its declaring-class short when that
variant is encountered. The native structures keep their original sizes.

Type size retains all six callees and its pointer/member-pointer dispatch.
Array multiplication is explicitly unsigned before the original signed
comparison, preserving 32-bit wrap without C signed-overflow undefined behavior.
On overflow the original updates the bound to one but returns the product
computed before that recovery; the C retains this ordering. Base sizing uses
complete-object size, while base-subobject layout uses nonvirtual size.
The member-layout checkpoint has 253 exact ordered frontiers. The following
nine functions implement packing/alignment, width-aware overflow checks, member
offset arithmetic, chain insertion, anonymous-class checks and access labels.
The class context names its last-member pointer at +4. The four special-member
requirement bits live at ClassInfo +0x37; their shared diagnostic descriptions
remain a contextual indexed table. Arithmetic bounds are defined using the
target limits for byte, short and long widths, with the original reserved slots;
all three 24-byte tables are audited against the original.

All 262 frontiers now replay exactly through the insertion/access prefix,
including pending FILE buffer contents. The next paired trap is find_outer_name,
which enters the lookup of the constructor name after the public label. No
complete frontend output is claimed at this still-partial frontier.

The following scoped-lookup prefix adds outer-name/scoped/class lookup, member
path setup, category matching, declaration-alias resolution and deferred token
capture. Class lookup uses typed qualifier paths and preserves ambiguity
reporting before retaining its first candidate. Three distinct class-lookup
state objects retain their original defaults and byte-address identities;
ordinary and class lookup continue to share the existing last-lookup scope and
scope-mode objects. OverloadSet has one shared 16-byte definition.

Alias resolution preserves qualified wrappers and updates their selected target;
its overload helper retains the full function-list/type/default-merge ABI.
Capture writes token records before checking delimiters. Raw EOF retains the
capture flag, while braces, a top-level colon, semicolon, unmatched closing
parenthesis or the explicit stop flag clear it. The 200th token sets the limit
flag after its record is written; reserved record bytes are left untouched.

All 270 ordered frontiers replay exactly through the scoped-lookup/capture
prefix in evidence/replay-scoped-lookup.log. The matched current frontier is
resolve_parenthesis_candidate at original 0x19190.

The separate declaration-lookahead parser (original 0x75230..0x75a52, semantic
switch 0x75a54..0x75ce8) is reconstructed from lookahead.bnf. Its 169 named
productions regenerate all 279 original state identities. Exhaustive audits
check 71,703 token actions, 14,229 gotos, lexer timing, and all 46 non-default
semantic action rules. Only logical generated rows enter the native build;
no original packed tables are included in the reconstructed implementation.

The parenthesis dispatcher preserves its ten callees, recursive cursor/depth
restoration and declaration/expression/constructor classification. The auxiliary
parser has 150 allocated semantic and state cells, plus original admitted
boundary aliases. Typed union views retain yypv/yys before-first cells and the
semantic/state overlap. Empty productions can read the following semantic cell,
so that view includes the overlapping state bytes as well.

Native references prove declarator_nesting is a signed 32-bit counter, correcting
its earlier provisional short declaration. Its canonical object and initial
byte-address audit now cover the full word. Deferred type conversion's second
argument is now typed as CType*, matching the token node's type field.

All 274 accepted frontiers replay exactly, including pending FILE buffers,
through this complete lookahead-parser integration. The matched current frontier
is begin_parameter_scope at original 0x36670. No complete-case equivalence is
claimed yet.

Constructor prototype reconstruction now includes parameter-scope creation,
deferred bucket allocation, parameter-chain insertion, constructor storage
checks, modifier merging, constructor dispatch and implicit-class pointer types.
The active parameter chain at 0x8c400 is a distinct object from the preserved
parsed-parameter chain at 0x91168. Scope retirement and popping remain separate
functions; deferred scope creation preserves the native high flag byte.

The reached constructor branch exposed an earlier unvisited parser action's
layout error: rule 88 used ClassReference +12 for a CType pointer. Native block
0x27680 reads CType +8. The typed access is corrected and compile-time offset
checks guard both layouts. Related rule 117 legitimately reads kind-15 class
references at +12; it was independently verified and remains unchanged. The
proof and paired/full-replay result are in evidence/constructor-type-index.txt.

All 284 frontiers replay exactly through the constructor/parameter prefix.
The next trap is configure_member_function at 0x2fef0. Accepted source-function
and switch-table ranges now cover 94,645 original .text bytes, excluding the
separate exact CRT/import frontier ranges; this is not executed-block coverage.

Member-function completion through the end of the Counter member list adds
configure_member_function, classify_constructor, overrides_virtual_member,
validate_function_default_arguments and make_ellipsis_parameter. The dispatcher
retains thirteen direct callees and its three-stack-slot stdcall ABI; full
virtual lookup retains both calls, all base/table passes and the four-string
return-type diagnostic. Virtual table entry stride, count/entry offsets, group
vector offset and ClassInfo vector offsets are guarded in the native build.
CppSymbol member flags and constructor qualifier bits share their original
bytes at +0x32/+0x33. The former empty_parameter_declaration placeholder is an
ellipsis marker allocator, with the untouched arena bytes and copied storage
preserved rather than clearing the record. Source ranges now total 96,634
original text/switch bytes; this is not executed-block coverage.

All 289 accepted frontiers replay exactly in evidence/replay-member-functions.log.
The matched current trap is end_class_specifier at 0x2dfc0. The complete frontend
case is still partial; no complete output equivalence is claimed.

Class completion now retains its eleven direct callees, including separate
scope retirement/pop and final member-name indexing. The conversion-operator
sets at ClassInfo +0x18/+0x1c were identified through the kind-17 OverloadSet,
its declaration flag 0x4000 (grammar rule 116), and canonicalization of the
selected conversion function's return type. Their iterators preserve untouched
padding and result fields until advancement. Permanent inline vectors provide
the shared representation and canonical empty-vector identity; no duplicate
empty-set global was introduced. Empty-set sharing still calls vector release,
which computes the original allocation size and calls the separate no-op
release_permanent entry (RET 8), preserving both graph edges and its ABI.

Virtual-layout preparation retains distinct initialization and construction
passes. The initialization clears inherited layout flags and allocates three
separate empty arena-two vectors for vf tables, vb tables and secondary table
groups. Native compile-time layout checks cover their ClassInfo offsets 8, 12
and 16, as well as conversion-set offsets 24 and 28. All original state defaults
and aliases remain covered by audit-data.py.

All 300 accepted frontiers replay exactly in evidence/replay-class-completion-setup.log,
including pending FILE buffers. The matched next trap is new_empty_vector at
0x4e800. Accepted source/table ranges now cover 97,828 original .text bytes;
this remains partial extent coverage, not executed-block or complete-case coverage.

Virtual-table construction retains all seven direct callees, including base
inheritance, vfptr/vbptr creation and their distinct table builders. The virtual
member test preserves overload traversal and its override-query edge. Closing
the nonvirtual size retains pending bit-field-unit rounding before the existing
alignment function and stores the result separately from complete-object size.

The implicit special-member driver uses four 28-byte local state records.
Their word view retains native indexed prefix accesses when an associated-this
model is -1. Its named fields and qualifier offset are checked using the original
compiler. Base and field passes preserve their subobject-helper edges, visible
member filters and the native replacement (rather than accumulation) of class
flag bit three. Class-valued member recognition, const qualification and direct
reference checks remain separate functions.

Native callsites at 0x3685c..0x368c1 pass seven stack slots to the completion
helper, which returns with RET 28 at 0x369c7, 0x36a49, 0x36abf and 0x36ad9.
The last argument is an unused count in this build, despite being omitted by the
decompiler; the reconstruction's typed declaration/stub retains it. Diagnostics
also retain their native class-kind/name or class-name argument lists.

The declaration-inspection pass retains overload order, constructor/assignment
counts and both child calls. Explicit member access values identify the flag
as nonprivate (access 1 is private), correcting its earlier provisional name.
ClassInfo's final word now has one canonical trait union for presence,
nonprivate access, ambiguity and cv qualification. Its byte view explicitly
retains the model -1 qualifier access to the preceding ambiguity byte.

Special-member completion preserves seven stack slots, its unused final count,
all declared/trivial/nontrivial/disabled/unavailable branches, class-name warning
arguments and the implicit-creator edge. The complete ClassInfo byte view keeps
indexed trait-prefix accesses within the owning object. Native checks establish
the 60-byte class-info size and final trait word at offset 56.

All 312 accepted frontiers replay exactly in evidence/replay-special-member-classification.log,
including pending FILE buffers. The matched next trap is implicit-member creation
at 0x374d0 (copy constructor for this case). Accepted source/table ranges total
100,167 original text bytes; complete-case equivalence is still unclaimed.


Implicit declarator creation and copy parameters now pass, retaining all seven
creator switch branches, the interned generated names, circular parameter links
and the reference merger's two eight-byte aggregate arguments. Shape replacement
preserves its contextual mutable prefix; the shared workspace defaults and
address are audited. The full overload binder and matcher preserve their 14 and
10 original callees, return/parameter distinctions and diagnostic arguments.
Details and ABI findings are in evidence/implicit-member-overloads.txt.

All 320 accepted frontiers replay exactly in evidence/replay-copy-overload.log,
including pending FILE buffers. The next matching trap compares declaration
parameter lists at 0x3a2d0. Accepted source/table ranges total 103,144 original
text bytes; the complete frontend case remains partial.


Implicit copy-constructor, copy-assignment and destructor classification now
passes. Parameter-list comparison preserves the accumulated exact/ambiguous/
distinct result; overload insertion uses the original function chain. Destructor
setup uses the shared internal int type, confirmed at its initialization store.
The final special-member model search keeps its native masked shift and trait
slot rules. Virtual finalization preserves both native passes and the first
pass's nine child calls; the previous-base pointer's BSS default is audited.

All 330 accepted frontiers replay exactly in evidence/replay-special-member-finish.log,
including pending FILE buffers. The next matching trap is direct-base layout
at 0x79dc0. Complete frontend output equivalence is still unclaimed.


The complete virtual-finalization path now passes: base/size completion,
virtual-base offsets, per-method override resolution, ambiguity diagnostics,
owner-path coalescing, permanent vector copies, abstract-class classification
and table-symbol declarations. The full 56-byte table and 36-byte group layouts
have native size/offset checks. Vector copying retains its indirect two-slot
callback ABI; unvisited table-copy callbacks remain named trapping stubs.
See evidence/virtual-layout-completion.txt for native ABI/data findings.

All 342 accepted frontiers replay exactly in evidence/replay-virtual-layout-finish.log,
including pending FILE buffers. The next matching trap is constructor-closure
processing at 0x37070. Accepted source/table ranges total 106,472 original text
bytes; complete frontend equivalence remains unclaimed.
