C1XX3232.EXE, Microsoft C++ frontend, MSVC 1.0 NT / project vc1.0

Target: toolchains/1.0-nt/bin/C1XX3232.EXE
SHA256: 333d015dd85773df2f0f9ab541ca65c40da9a2ad9bc8df9e03815f7deb20b2f7
Original image base 0x10000; entry point VA 0x804e0.
Starting case: original CL.EXE /Od /c cpp-basic.cpp. The independently verified
frontend input/output oracle is ../fixtures/cpp-basic/manifest.json.

PARTIAL binary reconstruction: 792 accepted frontiers, including 699 frontend
functions and seven older CRT routines. The cpp_basic starting case now completes
with exit0, empty stdout/stderr and exact raw intermediate files. All reached
application functions have readable C bodies and verified original call targets;
unvisited application callees remain named trapping stubs. Coverage below is
accepted original code extent, not branch coverage or full binary equivalence.
The approved inheritance/virtual/copy/destruction/allocation queue follows this
completed case; its durable record is cases/reconstruction-queue.json.

Native regression commands (run from this binary directory):

    make -j4 check
    make -j4 replay
    make -j4 native-check
    make -j4 native-rejections

check and replay use the instrumented i386 Linux binary and cached original
application first-entry history. native-check is the output-only variant.
No fixture invokes Wine, a PE helper, downstream compiler, or reference refresh.
functions.json retains every accepted original range/order; cases/native-traces.json
projects the original history onto all1036 application definitions, excluding
CRT/import entries on both sides. The case expects553 first application entries
and successful exit0. Explicit runtime evidence retains each library/import trap
rather than silently treating an unmatched terminal as an application entry.

Visible streams and all four intermediate files are compared byte-for-byte:
irex1136, irsy123, irin9 and irgl137bytes. Their hashes equal the independent
original phase oracle. Explicit acceptance comparisons also check four pending
FILE buffers and fourteen selected state values; the cached native regression
checks the complete first-entry trace, exit status, streams and final files.

The native final link compiles shared lib/recon C directly, without an archive.
Only application objects are instrumented; reconstructed runtime.c and shared
shims are excluded. Prefix/seen state is explicit and shared-FD ready; this
case's prefix is empty and it has no children. Every process request is rejected.
Only native compatibility is supplied by lib/recon; Win32 APIs, original source
ABIs, layouts, and function bodies are preserved.

Explicit reference/compiler operations are separate:

    make -j4 win32
    make -j4 audit
    make -j4 reference-capture

The default all target builds native-trace. win32 builds with the original
toolchain in the private Wine prefix. audit reads
both canonical binaries to check function graphs, ABIs and contextual data;
it does not execute either target. reference-capture executes only the masked
original in a new scratch directory and never updates checked-in references.
The old paired Wine regression and fallback runners have been removed. Historical
receipts/logs below remain evidence of the acceptance process that produced the
current checkpoint. Scratch lives under ~/.ai-tmp/vc1.0-c1xx.

The only inline assembly currently used in native source is INT3 in named
unreconstructed stubs, as required by the reconstruction protocol. Native
implementations use readable C; original CRT code is linked from the original
library after exact matching. No executable data sections are copied into C.

The original CRT __fmode is 0x8000 at VA 0xaa3bc. BINMODE.OBJ supplies that
same binary-default setting to this reconstruction; scripts/win32_indexes.py checks the
original and rebuilt globals against the recorded value. The PCH configuration
snapshot is represented by the typed CompilerConfiguration structure, with an
asserted size of 764 bytes. Its two path buffers and known flags are named;
remaining fields retain offsets until their consumers are reconstructed.

The launcher preserves the captured argv[0] identically for both images,
independently of the selected application image. scripts/audit-data.py checks all 98
option rules, their target sharing and defaults, the typed configuration, 81
language keywords, four __declspec entries and all 191 IR record schemas against the original binary.
The older CRT _sopen, setvbuf, _freebuf, fseek, __tzset, _isindst and ftell bodies have readable C replacements;
scripts/audit-runtime-variants.py and runtime-variants.json record their complete code
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

The parser's declarative grammar is recovered in src/grammar.bnf. scripts/grammar.py reads
only that named grammar, precedence and recovery annotations; it never reads an
original binary. Sorted LR kernels and queued closure regenerate the original
state numbering. LALR lookaheads, ordinary operator precedence and six comma
boundary overrides regenerate its actions and default reductions. Recovery
inserts identify production/dot contexts; destination states follow from shifts.
scripts/audit-parser-grammar.py compares all 147775 token decisions, 82800 gotos, lexer
query timing and 60 recovery inserts against the original. All match. The
analysis-only recovery proof is in scripts/analysis/recover-grammar.py; it is not part
of the build. The whole semantic switch is readable C in parser-*-actions.inc;
the complete C++ case remains pending.

The yacc stacks share their boundary storage with macro_work_midpoint. The
220-state array admits one extra halfword that aliases the first semantic
value; the 220-value array admits one extra eight-byte cell that aliases the
macro pointer and following control word. These overlaps use one C union.
The stack and six parser defaults are audited; all 207 frontiers replay with
these aliases in evidence/replay-parser-state.log.

scripts/generate-parser.py emits logical C tables from that same declarative grammar,
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
switch 0x75a54..0x75ce8) is reconstructed from src/lookahead.bnf. Its 169 named
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
and aliases remain covered by scripts/audit-data.py.

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


Class completion now passes constructor-closure/deleting-destructor drivers,
special-member lookup and both list/indexed search paths. The shared search
object is 16 bytes with checked offsets. The final index builder preserves its
six native callees and original temporary/arena/permanent allocation paths,
packing declarations in order within each interned-name bucket. Its sizing
helper retains cdecl. See evidence/class-member-completion.txt.

All 349 accepted frontiers replay exactly in evidence/replay-class-member-completion.log,
including pending FILE buffers. The next matching trap is deferred scanned-text
flushing at 0x1e7a0, reached by the class-declaration reduction (rule 48). Out-of-class
function bodies have not yet been accepted as parsed; equivalence remains partial.


Scanned-declaration flushing exposed and corrected a startup state error: the
initializer-record stack at 0x90f4c and deferred-inline stack at 0x8cfd0 are
separate objects. Startup initializes only the former. The complete flush helper
retains ten native callees and its replay/discard branches; unreached sealing,
pushback-reset and storage-formatting children remain named traps.

The differential harness now checks eight selected runtime fields and the
logical caller at each trap, in addition to exact output bytes and FILE buffers.
Only pointer nullness is compared for the two stacks; six scalar fields are exact.
A caller difference is accepted only for a proven native tail-jump chain reached
from the actual saved CALL return address. This accommodates original tail calls
without inventing assembly in rebuilt C. Evidence and the corrected first flush
caller are in evidence/scanned-stack-separation.txt.

All 350 accepted frontiers replay exactly with these stronger checks in
evidence/replay-scanned-stack-separation.log. The next matching trap is empty-
declaration checking at 0x3b9a0. Complete frontend equivalence remains partial.

Qualified constructor lookup is now reconstructed through recursive class-name
search and candidate filtering. The ten new functions preserve separate tag,
inherited-class reference, enum and overloaded-function representations. Qualifier
nodes are twelve bytes; class-search state is twenty bytes, and the next native
virtual-base iterator writes twenty-four bytes. Its trailing two fields are kept
even though this caller reads only the selected class.

All 360 accepted frontiers replay with exact streams, visible and pending file
bytes, selected shared state and logical callers in
evidence/replay-qualified-class-search.log. The next trap is virtual-base search
initialization at 0x7c060. The accepted executable extent union is 120,634 of
476,418 bytes (25.321%); this includes matched CRT/import ranges and shared text,
clipped and unioned against original executable VirtualSize. It is extent
coverage, not branch coverage. Complete frontend equivalence remains partial.

Function-definition entry is reconstructed as a whole function (0x339c0 through
0x33ff3), preserving its 24 call targets, recovery branches, implicit object and
structure-return parameters, storage diagnostics and browser declarations. Its
five newly encountered shared globals retain distinct original storage and have
checked initial values. The class's earlier declaration now passes through the
recovered default-argument merger, compatibility dispatcher and shape-chain
reconciler, retaining their three, seven and four native call targets.

Based-type exceptions use the shared segment symbol index at +0x10 (predefined
indices 5 and 2). Analysis of the 52-byte kind-13 segment allocator at 0x54f00
establishes that prefix; the separate segment-kind byte is at +0x24. No segment
allocator has been accepted merely from this analysis.

All 370 accepted frontiers replay exactly with pending buffers, selected runtime
state and logical callers in evidence/replay-function-definition-entry.log.
Current matching trap: equivalent shape attributes, native 0x610a0. Recovered
extent union: 123,614 / 476,418 executable bytes (25.947%; not branch coverage).
Complete cpp-basic frontend equivalence remains partial.

Constructor definition preparation now passes type, storage and DLL/linkage
reconciliation, scope activation, exit-label registration and result-type
construction. Storage merging retains nine native callees; preparation retains
six, and return-type construction retains five. Labels are ordinary 52-byte
symbol records with 20-byte references, both allocated in function arena one.
The based-type and shape-removal helpers preserve the original mutable copies
and allocator selection. Local-static guard state and the default inline
candidate are separate canonical objects, checked against original defaults.

The differential harness now compares the shared symbol-index word as well,
for nine runtime observations: two pointer nullness checks and seven exact
scalars. All 383 accepted frontiers pass with these checks in
evidence/replay-function-preparation.log. The call-graph audit also decodes past
each accepted endpoint to reject truncated final instructions, including RET n
operands; all 314 source extents pass. No C1XX extent correction was needed.

Current matching trap: begin_function_ir at native 0x3d6a0. The constructor body
is not yet parsed. Accepted executable extent union is 125,999 / 476,418 bytes
(26.447%, not branch coverage), and full frontend equivalence remains partial.

Frontend work-record construction is now recovered: common headers are sixteen
bytes (next +8, previous +12), symbol records twenty bytes, and function records
twenty-four bytes with their symbol at +16. Allocations use 16/32/48/64-byte
classes, forward word/byte copying and a reusable-record chain. Function entry
retains its list rotation, nontrivial parameter cleanup and empty-scope removal.
The allocator preserves seven native callees, including the matched original CRT
_setjmp used by its overflow recovery path. The original header defines an
8-word/32-byte jmp_buf; no assembly replacement is introduced.

The context reset uses the existing parser initializer_list_started object at
0xa0a24. Its reference-initialization predicate at 0x55e70 tests shape bit0x40,
so its earlier unaccepted provisional array name is corrected to
symbol_has_reference_shape. The distinct syntax diagnostic counter at 0x914cc
is shared by native parser reporters and record-recovery guards; those reporters
remain trapping stubs until reached.

All 390 accepted frontiers replay with exact files, streams and pending buffers,
logical callers and ten runtime observations (two pointer nullness checks and
eight exact scalars). Both symbol numbering and IR-record sequence progression
are now compared directly. Evidence: evidence/replay-frontend-records.log.
Current matching trap is identifier_expression at 0x3c330, while reading value
in the constructor's value_(value) initializer. Recovered extent union is
127,447 / 476,418 executable bytes (26.751%, not branch coverage). Complete
frontend equivalence remains partial.


The constructor initializer's identifier expression is now reconstructed through
symbol lookup, qualified/static/inherited resolution, symbol validation, storage
reference construction and argument-list append. Whole functions retain their
browser, based-pointer, permanent-storage and diagnostic branches, with named
traps for unvisited callees. The ordinary/qualified symbol wrapper shares its
actual CppSymbol ABI; member expressions instead consume IrToken operands.
Compile-time assertions distinguish their type fields at offsets 12 and 4.

IrToken's word at +8 has contextual views for the existing serialized record
index and the expression list's circular head link. Constructors preserve the
original untouched fields, including deferred member/overload type completion.
The recovered argument-list builder also sets its contained expression's link.
Evidence: evidence/identifier-expressions.txt.

All 400 accepted frontiers replay with exact streams, files, pending buffers,
logical callers and ten runtime observations in
evidence/replay-identifier-expressions.log. Current matching trap is
validate_value_expression at 0x3b280, while reading value in the constructor's
value_(value) initializer. Recovered executable extent union is 129,119 / 476,418
bytes (27.102%, not branch coverage). Complete frontend equivalence is pending.


The constructor initializer now passes value validation, binary/list-node
construction, list append and frontend symbol-record detachment. Its handoff
starts function-body output while retaining the initializer pointer for the
subsequent body pass. Original convention/prototype/debug branches remain C,
with named unresolved children. IrToken's final word now has typed binary-right
and string-metadata views, preserving its 20-byte layout.

The parameter count at original 0x91368 is the unsigned-short field of the
existing function_ir object. The address audit rejected a provisional separate
counter before acceptance; body setup and serialization share that state. The
older ftell implementation is independently matched against original C1 across
121 instructions, branches, calls and data aliases. It has readable C source.
Evidence: evidence/constructor-body-entry.txt and runtime-variants.json.

All 408 accepted frontiers replay with exact files, streams, pending buffers,
logical callers and eleven runtime observations (three pointer nullness checks
and eight exact scalars). Evidence: evidence/replay-constructor-body-entry.log.
The current matching trap is record_line_information at 0x5b0f0, the function
output preamble's source/option control-record writer. Accepted executable extent
union is 131,080 / 476,418 bytes (27.514%, not branch coverage). Complete frontend
equivalence remains pending.


Source/option control records now retain the full native field dispatch and
23-entry switch table. All 36 logical field schemas and eight contextual state
defaults/identities are audited. The counted-data consumer establishes only its
signed count byte; the still-unreconstructed producer will establish capacity.
Index and NUL-terminated filename encoders preserve byte promotion and their
individual function graph. The complete warning-override serializer is C.

The implicit this resolver preserves its circular parameter list, class-type
attachment and original diagnostic name arguments. Unary dispatch retains C++
overload lookup, builtin fallback and flag-restoration exits; clearing flag 0x10
preserves the DWORD object's upper half. Operand classification reuses the
already accepted type helper rather than introducing another function. The
next helper's provisional postfix name is corrected to builtin_unary_expression.
Evidence: evidence/control-unary-entry.txt.

All 415 accepted frontiers replay with exact files, streams, pending buffers,
logical callers and eleven runtime observations in
evidence/replay-control-unary-entry.log. The next matching trap is
builtin_unary_expression at 0x5f210, while forming the constructor's this
operand. Accepted executable extent union is 134,577 / 476,418 bytes
(28.248%, not branch coverage). Complete frontend equivalence remains pending.


The complete builtin unary helper retains member/function name resolution,
address-taking, constant folding, callable operands, indirection and optional
inline-analysis branches. Its 29 native callees match the rebuilt graph, with
named unresolved children. Operand checks, special-indirection diagnostics,
pointer-type recovery and left-typed node construction are also accepted. The
constructor's implicit this operand now reaches expression-tree output.

The source has checked 12-byte tagged x87 values, 14-byte inline-scope records
and 16-byte serialized inline records with their encountered slot bytes. It
shares the existing serialized stack/index and based-pointer state. The
member-pointer constructor's provisional signature is corrected to accept a
QualifiedDeclarator; parser calls preserve its stdcall ABI. Evidence and the
native unrecognized-operation fallthrough are in evidence/builtin-unary.txt.

The historical full-oracle history ran four isolated frontier pairs;
--jobs selects one to eight pairs. Each pair invokes the full comparator, with
private masked images, working folders and event/output logs. Every frontier
still checks exact files, stdout/stderr, pending FILE buffers, logical callers
and all eleven runtime observations. Frozen source, build, harness, original
target and fixture hashes are verified before and after the final ordinary
comparison. Those paired runners are retired; the immutable receipts remain.

All 420 frontiers pass in evidence/replay-builtin-unary.log (102.118 seconds),
with the frozen receipt in evidence/replay-builtin-unary-receipt.json. Current
matching trap is emit_expression_tree at 0x59b90. Accepted executable extent
union is 137,783 / 476,418 bytes (28.921%, not branch coverage). Complete frontend
equivalence remains pending.


Expression-tree serialization now retains the whole native recursion, constant
folding, void casts, function entry, hidden-parameter flags, argument lists and
switch chains. Its record meanings are contextual overrides of the declaration
vocabulary; all 213 native meanings are audited. Symbol indexing retains the
C++ member-context and alias edges with canonical reference and inlining state.
Typed-record output retains pointer/member-pointer, aggregate, enum, floating,
segment and void representations, qualifiers, sizes and alignment. The separate
expression alignment entry preserves its own identity and checked class offset.
Evidence: evidence/expression-output.txt.

All 427 frontiers pass with exact streams, visible files, pending FILE buffers,
logical callers and eleven runtime observations. The frozen four-pair receipt
is evidence/replay-expression-output-receipt.json and its ordered log is
evidence/replay-expression-output.log. Current matching trap is
emit_parameter_declaration at 0x34740, after matching function-entry output.
Accepted executable extent union is 140,173 / 476,418 bytes (29.422%, not branch
coverage), with 358 source functions. Complete frontend equivalence is pending.


ABI parameter declarations now emit their records and canonical scope-cleanup
symbol/depth pairs. The shared work-list allocator preserves permanent and
arena allocation, including the -1 pool selection. Compound entry retains its
separate native wrapper and whole shared body, including optional QC/browser
work, lazy symbol scopes and all C++ special-member branches. The linked-record
marker and free-record allocator retain their original list rotation, size
classes, splitting and unrelated flag bits. Evidence: evidence/parameter-scopes.txt.

All 433 frontiers pass with the full comparator at every frontier in
evidence/replay-parameter-scopes.log; frozen source/build/harness/fixture hashes
and receipts are in evidence/replay-parameter-scopes-receipt.json. Current
matching trap is emit_constructor_initializers at 0x35f90, after matching ABI
parameters and constructor-body scope entry. Accepted executable extent union
is 141,046 / 476,418 bytes (29.606%, not branch coverage), with 364 source
functions. Complete frontend equivalence remains pending.


The constructor-initializer processor now retains its whole virtual/direct-base,
member, array and implicit-copy logic, diagnostics, access-state changes and
class-table setup. All 20 native callees match the C graph. Its pointer-type
helper preserves the unused stack argument and ushort result established by
the native caller and RET 4. Class preparation and special-member definition
traversal retain their canonical queue, overload handling and mutation order.
The pointer set entry at 0x4e440 accepts both type and class-symbol pointers, so
its provisional CType-only name/signature is now add_inline_pointer.

Member lookup consumes the matching initializer through a pointer-to-link and
retains optional detached-symbol restoration. Both runs now select value_(value)
and reach prepare_expression at 0x3e780. All 439 frontiers pass in
evidence/replay-constructor-initializers.log, with frozen hashes and receipts in
evidence/replay-constructor-initializers-receipt.json. Evidence and ABI details
are in evidence/constructor-initializers.txt. Accepted executable extent union
is 142,513 / 476,418 bytes (29.913%, not branch coverage), with 370 source
functions. The constructor body and complete frontend case remain unfinished.


Expression preparation now retains its diagnostic recovery, 32-byte original-CRT
jmp_buf, deferred declaration records, temporary values, marker records and
nesting cleanup. Its shaped-void branch retains the exact native condition.
The shared DeclarationFrontendRecord has checked 24-byte layout and payload
offsets. The old unaccepted report_expression_error placeholder now returns
its recovered IrToken pointer, matching the actual native return.

Initializer context shares its canonical symbol/reference/member flags. All
frontiers now compare fourteen runtime observations: four pointer nullness
checks and ten exact scalars, including active initializer state. Member access
retains this adjustment, pointer-kind replacement and bit-field metadata. Both
runs reach initialize_lvalue at 0x46040 with the same active member context.

All 444 frontiers pass the stronger full comparator in
evidence/replay-expression-preparation.log, with frozen receipts/hashes in
evidence/replay-expression-preparation-receipt.json. Detailed evidence is in
evidence/expression-preparation.txt. Accepted executable extent union is
143,787 / 476,418 bytes (30.181%, not branch coverage), with 375 source functions.
The constructor body and complete frontend case remain unfinished.


Lvalue initialization now retains its conversion success and diagnostic paths,
assignment construction and record emission. Separate expression/type rankers
preserve typed and untyped overload handling, scalar promotions, /J character
rules, indices and qualifiers, including named traps for unreconstructed child
conversions. The special destination helper reuses the canonical indirect void
type and retains its signed rank comparison. Details: evidence/initializer-ranking.txt.

All 449 frontiers pass with exact output/buffers and fourteen runtime state
observations in evidence/replay-initializer-ranking.log. Frozen hashes/receipts:
evidence/replay-initializer-ranking-receipt.json. Accepted executable extent
union is 145,241 / 476,418 bytes (30.486%, not branch coverage), with 380 source
functions. The current matching trap is classify_derived_type_conversion at
0x646f0. The constructor and complete frontend case remain unfinished.


Derived-shape and initializer conversion now retain reference binding, pointer
and member-pointer compatibility, array/function signatures, class copying and
qualifier propagation. The shared initializer-reference helper has its recovered
general name prepare_reference_initializer at both ordinary and default-argument
callers. Four additional functions preserve all native call edges and layouts.
Details: evidence/initializer-conversion.txt.

All 453 frontiers pass the frozen full comparator in 115.422 seconds, with
fourteen exact/nullness state observations and raw pending intermediate bytes.
Receipt: evidence/replay-initializer-conversion-receipt.json. Accepted extent
union is 147,448 / 476,418 bytes (30.949%, not branch coverage), with 384 source
functions. Both runs now reach builtin_binary_expression at 0x5e040. Complete
constructor and frontend equivalence remain pending.


The whole builtin binary-expression builder and operand converter now retain
arithmetic, assignment, pointer/subscript, conditional and member-pointer paths.
Their 101- and 69-entry native switch tables are contextual shared text extents;
the implementation uses readable C switches. Value-dependent warning paths
retain void, sign, qualifier and constant/precision handling and actual variadic
diagnostic arguments. Evidence: evidence/binary-expressions.txt.

All 457 frontiers pass the frozen full comparator in 119.136 seconds; receipt:
evidence/replay-binary-expressions-receipt.json. Accepted executable extent
union is 153,945 / 476,418 bytes (32.313%, not branch coverage), with 388 source
functions. Both runs reach check_type_conversion_warning at 0x60830 during the
constructor assignment. The constructor and full frontend case remain unfinished.


The constructor member assignment now serializes with matching bytes. Type-only
conversion warnings, modifiable-lvalue selection, statement output and constant
symbol folding retain their full native graph and checked typed layouts. The
19- and 42-entry switches retain only contextual shared text in the masked
original. Evidence: evidence/assignment-output.txt.

All 461 frontiers pass the frozen full comparator in 114.054 seconds, including
exact emitted/pending intermediate bytes and fourteen state observations.
Receipt: evidence/replay-assignment-output-receipt.json. Accepted executable
extent union is 156,405 / 476,418 bytes (32.829%, not branch coverage), with 392
source functions. Both runs reach emit_class_table_initializers at 0x7b710.
The constructor body and complete frontend case remain unfinished.


Class-table pointer and vtordisp initialization now retain primary/inherited
vectors and their named qualifier-path helpers. Counter's empty vectors pass;
virtual-inheritance branches remain unexercised. Scope exit preserves its separate
native tail-entry wrapper, implicit return/finalization hooks, scope retirement,
QC state and cleanup/ordering graph. Evidence: evidence/constructor-scope-exit.txt.

All 466 frontiers pass the frozen full comparator in 117.849 seconds, with exact
output/pending buffers, logical callers and fourteen runtime state observations.
Receipt: evidence/replay-constructor-scope-exit-receipt.json. Accepted executable
extent union is 157,120 / 476,418 bytes (32.979%, not branch coverage), with 397
source functions. Both runs reach end_compound_scope_record at 0x3e6b0 while the
constructor body closes. Constructor return/finalization and complete frontend
equivalence remain unfinished.


The whole return dispatcher now retains implicit constructor returns, ordinary
and aggregate conversion, return-type inference, temporary cleanup and errors.
Constructor returns use the native adjusted-this helper. Return cleanup retains
its own error context, region markers and optional preserved-value temporary.
The canonical function-shape cache remains 21 entries/84 bytes, with an explicit
boundary/default audit against the following pragma-values cell. Evidence:
evidence/constructor-return.txt.

All 470 frontiers pass the frozen full comparator in 130.314 seconds, including
exact output/pending buffers, callers and fourteen state observations. Receipt:
evidence/replay-constructor-return-receipt.json. Accepted executable extent
union is 159,260 / 476,418 bytes (33.429%, not branch coverage), with 401 source
functions. Both runs reach process_expression_records at 0x40fc0 within implicit
constructor return cleanup. Complete constructor/frontend equivalence is pending.


The complete expression-record cleanup walker now preserves scope, return,
label/goto and loop-transfer handling, including nested delimiter search and
error-context ownership. Its three dispatch tables are accepted shared text;
the encountered 36-byte delimiter-mate table has a separate exact data audit.
Symbol reference tracking preserves label diagnostics and its three canonical
SEH nesting counters. Typed record prefixes distinguish label symbols from
expression-owned cleanup labels. Function finalization retains unresolved-label
and return-path checks, deferred goto islands and record-state release.
Evidence: evidence/record-cleanup.txt.

All 478 frontiers pass the frozen full comparator in 119.982 seconds, with exact
output/pending buffers, callers and fourteen runtime state observations. Receipt:
evidence/replay-record-cleanup-receipt.json. Accepted executable extent union is
162,880 / 476,418 bytes (34.188%, not branch coverage), with 409 source functions.
Both runs reach emit_scope_cleanup_references at 0x53ee0 after implicit constructor
return/finalization. Scope retirement and complete frontend equivalence remain
unfinished; unused control-flow cleanup paths retain named trapping children.


Function-body closure now retains exit-label emission, special-member epilogues,
outer-scope retirement, symbol-output hooks and canonical context resets. The
end-definition wrapper preserves its separate native tail-entry reset child.
Label definitions share their checked auxiliary-byte scope/SEH layout with the
reference tracker. Scope cleanup consumes paired depth/symbol entries through
the separately reconstructed work-list pop; retained scopes preserve native
parent/depth ordering. Evidence: evidence/function-scope-close.txt.

All 484 frontiers pass the frozen full comparator in 119.761 seconds, including
exact output/pending buffers, callers and fourteen runtime observations. Receipt:
evidence/replay-function-scope-close-receipt.json. Accepted executable extent
union is 163,607 / 476,418 bytes (34.341%, not branch coverage), with 415 source
functions. Both runs reach finish_function_output at 0x55ac0. Constructor scope
and metadata serialization, remaining definition cleanup and complete frontend
equivalence are still unfinished.


Function-output serialization now retains label diagnostics, scope bucket/member
traversal and dependency deferral. Declaration output preserves class-member
passes, aliases, PCH/debug rules, reference diagnostics and record selection.
All PCH checks reuse their existing canonical state/option objects. Name output
uses its separate C++ decoration entry, warning/fatal limits and direct CRT
string/NUL writes. Its 992-byte shared workspace ends before the independently
encountered ten-name cache. Evidence: evidence/symbol-serialization.txt.

All 489 frontiers pass the frozen full comparator in 122.872 seconds, with exact
output/pending buffers, callers and fourteen runtime observations. Receipt:
evidence/replay-symbol-serialization-receipt.json. Accepted executable extent
union is 165,531 / 476,418 bytes (34.745%, not branch coverage), with 420 source
functions. Both runs reach mangle_symbol_name at 0x42be0 during constructor
symbol serialization. Underlying name decoration, remaining metadata/definition
cleanup and complete frontend equivalence remain unfinished.


The complete decoration classifier now retains all 101 operator dispatch positions,
member/access/storage forms and compiler-generated name codes. Its symbol wrapper
preserves nested/plain fallbacks, convention suffixes and context mutations. The
constructor's directory entry records exact intermediate offsets and flags in
typed twenty-entry blocks. Type alignment, materialized-index clearing, qualified
scope unwinding and native tail-entry symbol-table resets finish the definition.
Evidence: evidence/constructor-definition-close.txt.

All 500 frontiers pass the frozen full comparator in 127.122 seconds, including
raw files/streams/pending buffers, callers and fourteen runtime observations.
Receipt: evidence/replay-constructor-definition-close-receipt.json. Accepted
executable extent union is 168,961 / 476,418 bytes (35.465%, not branch coverage),
with 427 source functions. Both runs now reach inherited_symbol_expression at
0x3c470 in the next method's member expression. Constructor definition teardown
is complete; the assigned whole frontend case remains partial.


Implicit member resolution and the shared dot/arrow selector now retain qualified
identity, overload bindings, temporaries, type qualifiers, bit fields and native
diagnostics. The complete access predicate handles protection, friends, inherited
declarations and virtual paths. Explicit qualifier/base offset walkers preserve
separate object-member and pointer conversions. Encountered access diagnostics
use contextual objects with original defaults/address identity audits. Evidence:
evidence/member-access-offset.txt.

All 510 frontiers pass the frozen full comparator in 133.210 seconds, with raw
files/streams/pending buffers, logical callers and fourteen runtime observations.
Receipt: evidence/replay-member-access-offset-receipt.json. Accepted executable
extent union is 171,934 / 476,418 bytes (36.089%, not branch coverage), with 437
source functions. Both runs now reach binary_expression at 0x5de30 during the
next method's expression processing. Complete frontend equivalence is pending.


The complete C++ binary dispatcher now retains overload selection, builtin
fallback and short-circuit preparation. Common type choice, numeric/integral
constant conversion and range checks preserve all scalar/pointer/floating
branches, diagnostics and canonical option/type state. The encountered range
tables use limits.h constants with exact original byte audits; old floating
callees remain named traps. Typed return operand selection and initialization
ranking retain their separate native graph entries. Evidence:
evidence/increment-return.txt.

All 517 frontiers pass the frozen full comparator in 131.901 seconds, including
raw files/streams/pending buffers, callers and fourteen runtime observations.
Receipt: evidence/replay-increment-return-receipt.json. Accepted executable extent
union is 174,359 / 476,418 bytes (36.598%, not branch coverage), with 444 source
functions. The increment and return IR now match; both runs reach the forward
record search at 0x41ac0 during nonvoid method path checking. Complete assigned
frontend equivalence remains pending.


The complete numeric scanner preserves radix and suffix selection, overflow and
bad-suffix diagnostics, numeric-sentinel handling and the general character
reader's trigraph/line-escape paths. Typed literal allocation reuses the existing
arenas and canonical type-description stack. Rescan depth is the existing parser
stack word at offset 2204, with signed tests and an explicit layout assertion;
it is not separate state. Evidence: evidence/numeric-scanner.txt.

All 525 frontiers pass the frozen full comparator in 132.553 seconds, including
raw files/streams/pending buffers, callers and fourteen runtime observations.
Receipt: evidence/replay-numeric-scanner-receipt.json. Accepted executable extent
union is 175,771 / 476,418 bytes (36.894%, not branch coverage), with 452 source
functions. Both constructor and next() now complete their definition paths;
main's literal 41 is scanned and both runs reach reject_function_initializer at
0x266a0. Complete assigned frontend equivalence remains pending.


Main's local declaration now retains parameter-name conflicts, local extern
lookup, overload/alias reconciliation, local-static copies and intrinsic metadata.
Object storage keeps the canonical inline/cleanup state and its data-segment
default, with complete type-size/error paths. Initializer setup preserves its
native symbol return and resets contextual value/constructor-range state.
Evidence: evidence/object-declaration-setup.txt.

All 533 frontiers pass the frozen full comparator in 136.731 seconds, including
raw files/streams/pending buffers, callers and fourteen runtime observations.
Receipt: evidence/replay-object-declaration-setup-receipt.json. Accepted executable
extent union is 177,693 / 476,418 bytes (37.298%, not branch coverage), with 460
source functions. Both runs reach initialize_object at 0x46290 with Counter and
literal 41. Complete assigned frontend equivalence remains pending.


PAUSED by explicit user request. The separate initialize_object wrapper at
0x46290..0x462ba is accepted after the 533-frontier checkpoint. Its entry pair,
one native call edge, all audits and the subsequent ordinary comparison pass;
both programs trap at initialize_object_value@8 (0x462c0). Current accepted
extent union is 177,735 / 476,418 bytes (37.307%, not branch coverage), with 461
source functions and 534 frontiers. The last full frozen history remains the
533-frontier replay in commit 6ed56aa (37.298%); this last wrapper has not received
a new historical replay. Evidence: evidence/initialize-object-wrapper.txt.

No build, debugger or replay is pending. The larger initializer dispatcher is
still a named trap, with read-only Ghidra/raw analysis in the worker's scratch.
Resume only when explicitly requested: make -j4 check, then reconstruct
initialize_object_value@8 as a whole function and continue the established paired
process. Run the full frozen history at the next normal batch checkpoint.
The assigned frontend case remains incomplete.


Source layout: reconstructed C, headers, semantic-action includes and declarative
grammars live in src/. Harness, audit, grammar-generator and analysis Python
scripts live in scripts/. Run make and script commands from this binary's root;
build/, functions.json, runtime metadata and evidence/ remain here. Existing
historical receipts retain the original frozen path names as evidence of those
runs; they do not describe current source locations.


Native migration verification and measurements are recorded in
cases/native-all-validation.json. Existing frozen original evidence remains in
evidence/fast-replay-verification-receipt.json and
cases/native-cpp-basic/manifest.json; the historic receipt name describes its
capture mechanism, not a current executable dependency. No automatic recapture
or paired Wine fallback remains. Historical fast/full replay timing receipts
are preserved under evidence/ and are not native benchmark measurements.


Single-function native-workflow trial (paused)

initialize_object_value@8 is now whole readable C for [0x462c0,0x46589),
713 bytes. All25 distinct call targets and every original RET8/branch are
represented;14 newly encountered callees remain named INT3 stubs. The shared
runtime fallback offset at0x91594 is contextual data, and conversion bookkeeping
reuses the existing integer_constant_scratch at0x912e8. No next function began.

Fresh original-only evidence and native GDB both stop in symbol_construction_class
at0x482f0, with all4 pending buffers,14 checkpoint fields, visible files and
streams exact. The canonical original-compiler build,462 call graphs, data
defaults,575-state main grammar and279-state lookahead audits pass. Native
regressions match456 application first entries across925 audited definitions.
The old original534-frontier receipt is immutable; a hash-checked fresh original
extension records the one additional acceptance.

Accepted code extent union is178448/476418 bytes (37.456183%), up713 bytes or
0.149658 percentage points. This is not branch coverage. The frontend case
remains incomplete. Evidence and all split timings are in
cases/object-initializer-trial/validation.json. Native fixture checks took0.244
and0.236 seconds; explicit original captures took4.425 and2.521 seconds. Native
build was7.898 seconds, canonical incremental build3.661 seconds, static audits
4.730 seconds, and native GDB inspection0.195 seconds. Analysis/source translation
was not separately timed. No Wine execution occurs in native fixtures.

The native workflow made named debugging and repeated checks quick. Cache
provenance needed an additive original-reference extension, and broad native
header dependencies currently cause full native rebuilds after header edits. No
shared compatibility changes were needed for this function. Resume only when
explicitly requested, at symbol_construction_class@4 in the existing cpp_basic case.

Autonomous C++ queue resumed. symbol_construction_class is now accepted after
fresh original/native matching and full static audits. Current construction
frontier is initialize_class_object at 0x47ef0; larger C++ cases follow cpp_basic.

Constructor-prefix checkpoint: 542 accepted frontiers, 469 source functions.
Class object setup, runtime/static guard setup, constructor-argument packaging,
type unqualification and builtin operator fallback now pass fresh original/native
checkpoint matching. The active guard pointer starts at the existing shared
12-byte guard object; this alias and its byte mask are audited. Coverage is
179304/476418 original code bytes (37.635858%), not branch coverage. The next frontier is
requires_address_temporary; cpp_basic remains incomplete.

Constructor-dispatch checkpoint: 548 accepted frontiers, 475 source functions.
The complete1540-byte allocation/object-construction dispatcher, pointer type
creation, address-temporary predicate and class/model predicates pass fresh
original/native comparisons (four pending buffers, fourteen state observations,
streams and all visible files), canonical direct-call audits and cached native
entry traces. Full475-function graph, contextual-data/defaults and both grammar
audits pass. Accepted executable extent union is181301/476418bytes (38.055027%),
not branch coverage. cpp_basic remains incomplete at make_size_constant@4.
Per-function reference/native/debug/fixture timings and canonical/native build
receipts are in cases/cpp-basic-frontiers/0542-* through0547-*. The dispatcher
needed two missing stdcall declarations before canonical linking; no shared
shim change was required. Original reference execution is explicit and remains
outside the native fixture runs.

Call/overload-prefix checkpoint: 565 accepted frontiers, 492 source functions.
The full1332-byte call dispatcher now handles the original conversion, callable
class, function-pointer and member-call branches in readable C. Constructor
binding, argument counting, nonvirtual-call selection and overload allocation,
population, cursor/phase tracking and arity filtering pass fresh original/native
comparisons and cached native entry/output checks. The52-byte search/candidate
and12-byte rank layouts have explicit size/offset assertions. Ordinary search
calls have3arguments; explicit-candidate calls have4. Static raw-stack evidence
is preserved in0557-new_overload_search/abi-review.txt.

Full492-function graph, contextual-data and both grammar audits, native regression
and6boundary rejection checks pass; combined check wall time4.485seconds (parallel
aggregate, not split per audit). Accepted extent union184669/476418bytes is
38.761970%, not branch coverage. Evidence is in
cases/cpp-basic-frontiers/0548-* through0564-* and
evidence/constructor-overload-checkpoint-checks.{json,log}; per-function capture,
build/debug and fixture timings remain separate. cpp_basic remains incomplete
at rank_overload_arguments@12. No shared shim change was needed.

Constructor binding checkpoint: 586 accepted frontiers, 513 source functions.
The selected call now traverses standard/user conversion ranking, converting
constructor search, rejection, two-pass overload pruning and qualified binding.
Whole functions and all newly encountered callees retain their original ABIs;
new callees are named INT3 stubs. The optional conversion context is now an
OverloadCandidate pointer, and each12-byte rank record names its selected
conversion function. Size and offset checks preserve those recovered layouts.

Every acceptance0565through0585 has fresh original/native matching of all four
pending buffers, fourteen state observations, streams and visible bytes, plus
canonical graph and cached native trace verification. Both compilers build.
Full513-function graph, data/default and both grammar audits, native regression
and6boundary rejection checks pass in4.552seconds combined parallel wall time;
separate build/capture/debug/fixture timings remain in the per-function records.
Accepted extent union187398/476418bytes is39.334786%, not branch coverage.
cpp_basic is still incomplete at get_base_initialization_access (0x32060).
Receipts: evidence/constructor-binding-checkpoint.{json,log},
constructor-binding-native-check.json and constructor-binding-rejections.json.
The mutable current validation summary now refreshes its coverage/frontier on
every acceptance; historical frozen receipts remain unchanged.

Resolved-call checkpoint:595 accepted frontiers,522 source functions.
The complete resolved-call builder, explicit/default argument conversion,
member-object binding, trivial special-member simplification and result
finalization now pass fresh original/native checkpoint comparisons. Hidden
class-result storage, reference results and complete-object constructor
arguments retain their original branches and call graph. ClassInfo's dispatch
pointer is the symbol at offset20, checked explicitly; the generated-member
queue and base-initialization access flag share their original contextual state.

All acceptances0586through0594 match four pending buffers, fourteen state
observations, raw streams, visible file bytes, canonical call graphs and cached
native entry traces. Full522-function graph, contextual data/defaults, both
grammars, native regression and6boundary rejection checks pass in4.602seconds
combined parallel wall time. Per-function original reference, native debug,
build and fixture timings are retained separately in each case directory.
Accepted executable extent union190177/476418bytes is39.918097%, not branch
coverage. cpp_basic remains incomplete at finish_object_initializer (0x3dac0).
No shared compatibility change was required. Checkpoint receipts are
in evidence/call-result-checkpoint.{json,log}, call-result-native-check.json
and call-result-rejections.json.

Main-expression checkpoint:608 accepted frontiers,535 source functions.
Object initializer completion and member-object addressing now lead through
main's member call, integer promotion, numeric/pointer operand conversion,
constant folding, evaluation boundaries and the conditional operator. The full
constant folder includes all43 operation-dispatch entries and keeps floating
and overflow helpers as named traps until reached. Conditional cleanup preserves
its original right-alternative/left-alternative record processing order.

Acceptances0595through0607 have fresh original/native matching of four pending
buffers, fourteen state observations, raw streams and visible files, plus
canonical candidate graphs and cached native traces. Full535-function graph,
contextual-data/defaults, both grammar audits, native regression and6boundary
rejections pass in4.718seconds combined parallel wall time. Per-function capture,
build/debug and native fixture timings remain separate in each acceptance record;
repeated builds are retained as initial-build-timing.json where applicable.
Accepted extent union195525/476418bytes is41.040641%, not branch coverage.
Main's return has completed and cpp_basic remains incomplete at close_source_file
(0x1e990), before frontend teardown. No shared shim change was needed.
Receipts: evidence/main-conditional-checkpoint.{json,log},
main-conditional-native-check.json and main-conditional-rejections.json.
The approved follow-on families and capture constraints are durable in
cases/reconstruction-queue.json; capture starts after cpp_basic completes.

Source-close checkpoint:612 accepted frontiers,536 source functions.
The complete752-byte source-stack close routine preserves parent-window/handle
restoration, packing diagnostics, forced includes, PCH boundaries and source
locations. The newly encountered forced-include byte and source-name pointers
have default/identity audits. The source-name copy cursor is also the closed-file
name slot; no duplicate state was introduced.

The next three first-entry traps are exact LIBC __close, import:CloseHandle and
LIBC __free_osfhnd. Their member identity is verified against the original and
canonical link with exact nonrelocation bytes, named call relocations, shared
data targets and library/object hashes. Explicit original/native breakpoint
observations agree on descriptor7, open flag1, result0 and closed flag0 at the
source-close caller. The nested runtime sequence is observed and retained, not
silently removed from the history. These are library/import acceptances and
add no application source functions.

Fresh sequential masked-original captures after those acceptances and native GDB
both reach finish_translation_symbols with all four pending buffers, fourteen
state observations, streams and visible file bytes exact. The ordinary cached
native app trace also passes. Full536-function graph, contextual-data/defaults,
both grammars, native regression and12boundary/evidence rejections pass in
4.941seconds combined parallel wall time. Six new rejection checks cover wrong
close return, failed return on both sides, fabricated descriptor, runtime order,
library identity and observation hashes. The offline runtime validator does not
need a canonical executable or Wine; explicit canonical proof generation remains
a reconstruction operation.

Accepted extent union196555/476418bytes is41.256837%, not branch coverage.
cpp_basic remains incomplete before translation-unit symbol finalization.
Receipts are cases/cpp-basic-frontiers/0608-close_source_file through0611-*,
and evidence/source-close-checkpoint.{json,log}, source-close-native-check.json,
source-close-rejections.json. The runtime-boundary subdirectory contains the
exact original/native call observations and all split capture/debug/observer
build timings. The acceptance operation timer excludes those earlier captures;
the separately recorded original captures total10.287seconds, the extra boundary
capture2.469seconds and native boundary observation0.184seconds. No shared shim
change was required. Native fixtures execute no original/reference process.

To reproduce the explicit close observation outside fixtures, build
build/reference-close-observer.exe with make, capture the masked source-close
candidate with scripts/capture-original.py, then pass that scratch capture
folder to scripts/capture-close-boundary.py. The observer restores only the
reviewed indexed runtime/import bodies at their matching traps and traps again
at the caller's return. Its source is diagnostic code, never target implementation.

Translation-finalization checkpoint:620 accepted frontiers,544 source functions.
The complete deferred-special-member generator, declaration/type table walks,
queued virtual-table traversal, global/segment output driver and function-directory
serializer now pass fresh original/native comparisons. The directory restores
signed one-bit flags exactly and retains the original final-block lookup for
its saved optimization bit, even while traversing earlier blocks. Its shared
function-state word at+16 now names the observed out-of-line emission requirement,
distinct from the separate no-inline attribute and inlining-restriction state.

The scoped cleanup heads, declaration/type maps and segment list have contextual
default/identity audits. Every acceptance0612through0619 matches four pending
buffers, fourteen state observations, streams and visible bytes, plus canonical
candidate graphs and cached native traces. Full544-function graph, contextual
data/defaults, both grammars, native checks and12boundary/evidence rejections
pass in4.576seconds combined parallel wall time. All per-function build,
reference capture, native debugger and cached-fixture timings remain separate.
Accepted extent union198134/476418bytes is41.588269%, not branch coverage.
cpp_basic is incomplete at mangle_symbol_identifier (0x433b0) while serializing
its final function metadata. Evidence is in the0612through0619case directories
and evidence/translation-finalization-checkpoint.{json,log},
translation-finalization-native-check.json and translation-finalization-rejections.json.
No shared compatibility change was required.

Mangler checkpoint:632 accepted frontiers,556 source functions. Twelve whole
functions now cover identifier/scope caches, storage/type dispatch, cv qualifiers,
function conventions, parameter references and scalar/pointer/tag type dispatch.
Both ten-entry caches retain interned pointer identity, with explicit40-byte
bounds and initial-state checks. The function terminator and enum representation
helpers retain their original unused second ABI argument (RET8).

Every acceptance0620through0631 has exact fresh original/native traps, four
pending buffers, fourteen state observations, streams and visible bytes. The
full556-function graph, contextual data/defaults, both grammars, cached native
regression and12evidence/boundary rejections pass in9.534seconds combined wall
(including a canonical grammar rebuild). Per-function canonical/native builds,
original capture, native debugging and cached-fixture times remain separate.

One rejected comparison exposed a native migration omission: BINMODE.OBJ's
0x8000 default was missing from C1XX's native flags. Its reconstructed old ftell
therefore overcounted embedded LF bytes, producing saved offsets+1/+2 while all
mangled strings agreed. Native flags now use the existing shared binary-mode
option. Original __sopen data at0xaa3bc and canonical BINMODE.OBJ agree on0x8000;
the retained failure, proof and accepted correction are under
evidence/native-binary-mode and acceptance0631. No frozen capture was modified.
The corrective forced native rebuild was not separately timed; it is labeled
unmeasured rather than folded into the earlier per-function build timing.
No shared shim source change was needed.

Accepted extent union200712/476418bytes is42.129391%, not branch coverage.
cpp_basic remains incomplete at emit_function_symbols (original0x54940).

cpp_basic completion checkpoint:643 accepted frontiers,560 source functions.
The overload-group walk, dependency ordering, intermediate flush and incomplete
PCH cleanup now preserve their whole original bodies. PCH stream pointers and
ownership/status words have six initial-state/address checks. The error path's
_fileno remains an out-of-line call despite STDIO.H's default macro.

The flush boundary exposed one native compatibility difference: a successful
positive buffered write on an update stream clears its write flag. Exact entry
and return observations retained both the rejected138/138 flag transition and
the corrected138/136 transition. The parent fixed shared recon_flush with the
original write/buffer predicates and reset semantics. Repeated successful
observations, original-toolchain observer build and corrective build timings
remain separate in acceptance0634/runtime-boundary. Four rejection checks
exercise stale flags, failure on both sides and corrupted library identity.

Explicit original/native exit entry agrees on code0, four empty FILE buffers,
fourteen selected fields, streams and visible files. The original then follows
exit, doexit, endstdio, flushall, flsall and ExitProcess to exit0. Every library
body has exact relocation-aware LIBC member identity; doexit's static .data
references are anchored by the object's exported globals and initialized bytes.
Each original first-entry trap is retained in0636through0642, and the native
projection excludes only proved library/import entries. No termination range
was silently unmasked or counted as application source.

The cached native regression now checks553 first application entries, exit0,
empty streams and full raw bytes:irex1136,irsy123,irin9,irgl137. These hashes also
match the independent original phase fixture. Full560-function graph, contextual
data/defaults, both grammars and19meaningful native/evidence rejections pass;
combined parallel wall is4.817seconds. Detailed split timings, repeated captures
and explicitly unmeasured failed-boundary elapsed time are retained in
evidence/cpp-basic-completion-timing.json. Native fixtures execute no original.

Accepted extent union201713/476418bytes is42.339500%, not branch coverage.
This completes the starting case; the binary remains partial and the approved
follow-on C++ queue remains active.

Approved follow-on references are now captured and independently verified under
../fixtures/c1xx-virtual-diamond, c1xx-copy-assignment, c1xx-stack-lifetime and
c1xx-new-delete. They cover a shared virtual-base diamond and virtual dispatch,
copy/assignment/conversion/value return, nested/temporary/array destructor order,
and scalar/array allocation with class allocation and subscript operators.
Every original CL compilation and linked program succeeds with empty program
streams and exit0. Actual child argv0/environment transport, intermediate inputs,
object/response, source/header/error-file dependencies and exact hashes are durable.
Independent C1XX/C2/LINK replay passes all four; only demonstrated downstream
COFF/PE timestamps are excluded. Reference registration is an explicit script
operation, separate from native fixtures. Native per-case seed integration is
next; these captures have not yet been claimed reconstructed.

Multi-case adapter checkpoint:the completed cpp_basic history remains fixed at
553 application entries and exit0. virtual_diamond starts from the proved643
accepted prefix, seeds553 application names, and matches complete_declaration
(original0x518b0) with four pending buffers, fourteen fields, streams and files
exact. cases/virtual_diamond-seed retains both explicit captures; registration
accepted no additional function. Global cases/canonical-map.txt classifies all
current application definitions; per-case historical maps remain evidence.

Ordinary make check/replay now runs every registered case with its own source,
reference files, expected terminal and seen-state prefix. --case selects one
for native diagnostics/checks. Full canonical audits, both cached cases and21
rejection checks pass in4.679seconds combined wall. New rejection checks remove
main from the new case's seed and force a completed exit at its pending trap.
The case fixtures remain native-only. Full reference capture and acceptance
operations accept an explicit --case separately. The active frontier is now
complete_declaration; accepted extent remains42.339500% before its recovery.

Virtual-pointer layout checkpoint:651 accepted frontiers,568 source functions.
Eight whole functions beyond the virtual_diamond seed now bind declarations,
collect storage/modifier flags, synthesize a vfptr/vbptr and its nested type,
validate pointer attributes, prepend synthetic members, relocate data/base
members and iterate the class member chain. The storage dispatcher includes
its16-entry original text table; source uses a readable switch. Diagnostic
varargs retain symbol names/token spellings, and both unused fourth storage
arguments and original RET widths are preserved. The class iterator is8bytes
with its selected member at+4, checked by both compiler builds.

Every0643through0650 acceptance matches fresh original/native traps, four pending
buffers, fourteen state values, streams and visible bytes. cpp_basic still
passes its full original output/exit oracle at every step. Full568-function
graph, contextual data/defaults, both grammars, both cached cases and21rejection
checks pass in4.726seconds combined wall. Per-function build, original capture,
native debugger and fixture timings remain in the acceptance directories.
No new semantic divergence or shared compatibility change occurred.

Accepted extent union203249/476418bytes is42.661906%, not branch coverage.
The active virtual_diamond case is incomplete at new_virtual_table0x78950.

Virtual-entry storage checkpoint:664 accepted frontiers,581 source functions.
Thirteen whole functions beyond d4a4c33 construct the zeroed table, grow arena
vectors, record explicit special members, walk override candidates, select the
primary table, initialize entries/this adjustments, grow heap entry storage and
reserve a virtual slot's byte offset. The reached generic vector accessor is
now named first_vector_item. grow_vector returns its original Vector pointer;
all original stack slots, signed-short growth and diagnostic varargs remain.

The 16-byte VirtualEntry now contains an 8-byte VirtualAdjustment at+4 and a
single flag byte at+12. Original reserved bytes and high flag bits are retained.
Special-member model -1 accesses shift words within its 28-byte record.
Every0651through0663 acceptance matches fresh original/native traps, four pending
buffers, fourteen state values, streams and visible bytes. cpp_basic's completed
exit and four final intermediate outputs remain exact throughout. Full581-function
graph, contextual defaults/data, both grammars, both native cases and21rejection
checks pass in4.679seconds combined wall. Per-function build/capture/debug/fixture
times remain separate in the ordered acceptance directories. No mismatch or
shared compatibility change was needed in this group.

Accepted extent union204114/476418bytes is42.843469%, not branch coverage.
virtual_diamond remains incomplete at matches_virtual_override0x7a880.

Virtual realloc checkpoint:667 accepted frontiers,582 source functions.
The whole override matcher compares names, shared parameter declarations, this
pointer model/cv attributes and compatible return types. Its class slot remains
unused but RET12 is preserved. Its first completed path reached _realloc, where
ordinary acceptance correctly rejected a native later trap as an unaccepted CRT
seam. Explicit original-only captures establish _realloc then __heap_expand_block
before the matching clone_class_virtual_table trap. Both CRT bodies exactly match
LIBC members and canonical link relocations; they remain separate from application
function counts.

The explicit original/native realloc entry/return proof observes count1,
capacity3, request48 and nonnull return. Both sides preserve all16old bytes.
The first pointer is the independently read symbol named value; the other12bytes
match raw exactly. No padding normalization is used. Four pending buffers,
fourteen state words, streams and every visible file agree downstream. The new
observer is original-only; ordinary fixtures never invoke it. Runtime acceptance
now selects case-specific caches, prefixes and evidence directories.

Full582-function graph, contextual data/defaults, both grammars, both native
cases and25negative checks pass, including corrupted realloc size, preserved
prefix, member identity and observation hash. The combined command was not timed;
individual build/reference/GDB/fixture timings and the initial unmatched native
run are retained. The observer build elapsed is explicitly unmeasured. Accepted
extent union204709/476418bytes is42.968360%, not branch coverage.
virtual_diamond is incomplete at clone_class_virtual_table0x78a40.

Virtual-table permanence checkpoint:677 accepted frontiers,592 source functions.
Ten whole functions beyond62fc1ba persist table/owner/entry storage, release the
temporary heap table, construct its typed declaration, intern its decorated name,
encode owner scopes, and finish the array declarator. The clone's full110-byte
body ends at0x78aad; Ghidra's displayed range omitted its later blocks. Both
original callback slots remain, including the unused arena slot. Vector copies
preserve repeated count queries and fatal filename/line arguments. The table's
auxiliary pointer now has a contextual VirtualTable union member at+0x24.

Raw initialization proves original0x9f268 is type_0064_q1000, used temporarily
while naming the table. Original special-identifier targets select vftable or
vbtable, and table declaration retains both kind branches and DLL storage bits.
Every0667through0676 acceptance matches fresh original/native traps, four pending
buffers, fourteen state values, streams and every visible file; completed
cpp_basic remains exact. Full592-function graph, contextual defaults/data, both
grammars, both cached cases and25negative checks pass. Measured combined wall:
4.654033seconds.

Accepted extent union205621/476418bytes is43.159788%, not branch coverage.
The active virtual_diamond case remains incomplete at make_deleting_destructors.

Deleting-destructor alias checkpoint:683 accepted frontiers,598 source functions.
Six whole functions beyond76ce979 generate scalar/vector deleting destructors,
record their method alias and insert it through the generic hash dispatcher.
Access, virtual slot/table identity and this displacement are copied separately
into both wrappers. The virtual-base walk requests the additional destructor
wrapper for the first nontrivial base. Translation-wide method_aliases at
original0x9f334 is distinct from the per-function alias tables and has an audited
zero default.

The hash and equality callback arrays are contextual named dispatch tables,
including their original null sentinels. All ten hash and nine equality entries
retain separate original cdecl functions; unvisited callbacks remain named traps.
Their original/rebuilt pointer identity is now audited, alongside the existing
data/default/alias checks. Hashing retains its full unsigned callback result;
equality receives search key before stored key. The reached aligned-pointer
callback hashes only aligned low-page bits.

Every0677through0682 acceptance matches fresh original/native traps, four pending
buffers, fourteen state words, streams and visible files. Both cached native
cases, full598-function graph/data/grammar audits and25negative checks pass.
Combined measured wall: 4.750118seconds.

Accepted extent union206186/476418bytes is43.278382%, not branch coverage.
virtual_diamond has completed Base's class declaration and is incomplete at
add_named_base while beginning Left's inheritance declaration.

Virtual-base insertion checkpoint:688 accepted frontiers,603 source functions.
Five whole functions beyond f029b81 resolve a named base/tag and class-path leaf,
validate/allocate the base record, and classify repeated bases recursively in
both directions. All diagnostic branches and argument order are retained;
ambiguous-path diagnostics continue recovery while duplicate direct bases do
not insert. The lookup's tag_declaration_seen object remains shared with tag
allocation and class completion.

Original access arguments are full32-bit stack words, compared against zero
before only their low bits are stored. Corrected the earlier provisional byte
parameter in both parser entry declarations and base forwarding. The reached
allocation proves BaseClassLink is24bytes with its char*name at+8; compile-time
layout checks cover both original/native builds. Source preserves declaration
arena choice, access defaults, virtual inheritance flags and browser call edge.

Every0683through0687 acceptance matches fresh original/native traps, four pending
buffers, fourteen state values, streams and visible files. Completed cpp_basic
remains exact. Full603-function graph, contextual data/defaults, both grammars,
both cached native cases and25negative checks pass. Combined measured wall:
4.762676seconds.

Accepted extent union207634/476418bytes is43.582316%, not branch coverage.
virtual_diamond has inserted Left's virtual Base and remains incomplete at
compatible_override_return0x798f0 while examining inherited virtual methods.

Virtual-base table checkpoint:698 accepted frontiers,613 source functions.
Ten whole functions beyond321b685 compare override return shapes, union/iterate
inherited conversion sets, mark inherited virtual-pointer layout, query integer
type/base extent, and build the initial vbtable entry. Native AX loads followed
by full EAX masks return complete zero-extended words; the original getters and
width query remain separate calls. The base extent is the last visible direct
base end, retaining original list order rather than taking a maximum.

VirtualEntry is now a16-byte union: VirtualCallEntry contains its function,
8-byte this adjustment and flag byte; VirtualBaseEntry contains its vbptr offset
and base-class pointer with untouched suffix workspace. Both variants and their
field offsets have original/native compile-time checks. Their common append/copy
functions remain whole and preserve all16bytes. Multiple native reads/writes now
establish ClassInfo.virtual_base_pointer_offset at+44, including pointer shifting,
table construction and base-entry initialization. The full prior function graph
was rechecked immediately after this type refinement.

Every0688through0697 acceptance matches fresh original/native traps, four pending
buffers, fourteen state values, streams and visible files. No artifact bytes were
normalized; completed cpp_basic remains exact. Full613-function graph, contextual
data/defaults/callback identities, both grammars, both cached native cases and
25negative checks pass. Combined measured wall:
4.917202seconds.

Accepted extent union208177/476418bytes is43.696292%, not branch coverage.
virtual_diamond has created Left's vbptr and initial vbtable, and is incomplete
at merge_subobject_special_members0x36ee0 while inheriting special-member traits.

Inherited table-copy checkpoint:708 accepted frontiers,623 source functions.
Ten whole functions beyond bab3ed2 merge inherited special-member eligibility,
copy both virtual table families, create virtual-base groups and retain the
source table/direct path separately. The copy callback changes table kinds0/1
to2/3 and marks each inherited dispatch entry; its unused arena slot remains
part of the original two-argument ABI. Temporary copies reserve full entry
capacity and a spare owner slot while copying only used entries.

VirtualTableGroup stores its class pointer at+0 and initializes only the flag
byte at+28; the suffix padding is preserved. VirtualTable stores its direct
BaseClassLink path at+40 and source table at+44. Both original and native
builds check these layouts explicitly. Unreached merge/offset callees remain
named trapping stubs.

Every0698through0707 acceptance matches fresh original/native traps, four pending
buffers, fourteen state values, streams and visible files. Completed cpp_basic
remains raw byte-exact. Full623-function graph, contextual data/defaults/callback
identities, both grammars, both cached native cases and25negative checks pass.
Combined measured wall:4.842415seconds.

Accepted extent union209366/476418bytes is43.945863%, not branch coverage.
virtual_diamond remains incomplete at merge_virtual_base_group0x7a090.

Virtual-base eligibility checkpoint:718 accepted frontiers,633 source functions.
Ten whole functions beyond257caf5 merge shared virtual-base groups and determine
new/inherited vtordisp requirements. The predicate checks inherited slots first,
then suppression, non-destructor virtual entries, lifetime traits, layout
attributes and actual derived overrides. The overload scan preserves canonical
name/parameter/this comparisons and its separate return-type predicate.

Group object_offset at+4 and has_vtordisp at+32 now have contextual names and
layout assertions. The group adjustment body retains its unused third stack
argument and uses the group's stored offset, exactly as the original does.
Anonymous storage and other unreached callees remain named traps. An unused
preparatory draft build was removed before acceptance; its elapsed cost and a
missing-declaration build retry are recorded explicitly with frontier0717.

Every0708through0717 acceptance matches fresh original/native traps, four pending
buffers, fourteen state values, streams and visible files. Completed cpp_basic
remains raw byte-exact. Full633-function graph, contextual data/defaults/callback
identities, both grammars, both cached native cases and25negative checks pass.
Combined measured wall:4.781096seconds.

Accepted extent union210776/476418bytes is44.241821%, not branch coverage.
virtual_diamond remains incomplete at shift_inherited_virtual_table0x79f50.

Virtual-group offset checkpoint:728 accepted frontiers,643 source functions.
Ten whole functions beyond41ece83 adjust all four table kinds, select virtual
this identities, combine inherited access, locate/create primary vbtable entries,
retain override hooks, query offsets and persist complete groups. The first
virtual-base identity wins; later direct adjustments leave it untouched. The
original override hook is exactly RET8, retained as a separate empty C function.

Group offsets+8/+12 and pointer symbol+16 now have contextual names and both
original/native layout checks. The inherited-access4x4matrix is defined beside
its consumer with private/protected/public meaning; its16bytes are audited
against the original. Group copying preserves all36bytes and honors its arena
argument while its existing table-copy callback keeps its own distinct ABI.

Every0718through0727 acceptance matches fresh original/native traps, four pending
buffers, fourteen state values, streams and visible files. Completed cpp_basic
remains raw byte-exact. Full643-function graph, contextual data/defaults/callback
identities, both grammars, both cached native cases and25negative checks pass.
Combined measured wall:4.866464seconds.

Accepted extent union211509/476418bytes is44.395678%, not branch coverage.
virtual_diamond remains incomplete at place_base_subobject0x793c0.

Inherited-group merging checkpoint: 738 accepted frontiers, 653 source functions.
Ten whole functions beyond 19a5bb2 place direct base subobjects, inherit their
vfptr/vbptr layout, copy complete working groups through their separate vector
wrapper, allocate anonymous vtordisp space and merge shared dispatch entries.
The direct-base placer preserves repeated packing queries, empty-base separation,
leading/trailing empty flags and its computed unsigned offset result, despite
the analysis decompiler's provisional void signature.

Related declaring classes select the more-derived final overrider; unrelated
functions form a conflict vector. Complete this-adjustment records are copied
without changing their padding. The group merger retains its selected destination
table on iterator exhaustion, as the original does; this matching-group case
supplies corresponding tables. Unreached conflict helpers remain named traps.

Every 0728 through 0737 acceptance matches fresh original/native traps, four
pending buffers, fourteen state values, streams and visible files. Completed
cpp_basic remains raw byte-exact. Full 653-function graph, contextual data/defaults/
callback identities, both grammars, both cached native cases and 25 negative
checks pass. Combined measured wall: 4.826083 seconds.

Accepted extent union 213128/476418 bytes is 44.735505%, not branch coverage.
virtual_diamond remains incomplete at release_virtual_group 0x79000.

Inline-pointer growth checkpoint: 748 accepted frontiers, 663 source functions.
Ten whole functions beyond 744ceae release temporary groups, compute vbtable
byte indices, collapse conflict vectors, compare owner vectors and grow sorted
inline pointer sets. Their search orders low twelve address bits, then scans
an equal-key group for full identity; missing pointers insert at the lower
bound. The optional result-position store precedes the final count reload.

The growth wrapper stores its signed-short capacity before separate new/old
size queries. Permanent resize allocates a new block and copies the smaller
size through a distinct unsigned-minimum helper, retaining old arena storage.
All these function boundaries and cdecl/stdcall pointer-return ABIs remain
separate; no source implementation uses generated assembly.

Every 0738 through 0747 acceptance matches fresh original/native traps, four
pending buffers, fourteen state values, streams and visible files. Completed
cpp_basic remains raw byte-exact. Full 663-function graph, contextual data/defaults/
callback identities, both grammars, both cached native cases and 25 negative
checks pass. Combined measured wall: 4.964778 seconds.

Accepted extent union 213747/476418 bytes is 44.865433%, not branch coverage.
virtual_diamond remains incomplete at make_table_qualifier_path 0x7b9e0.

Virtual-base hidden-parameter checkpoint: 753 accepted frontiers, 668 source
functions. Five whole functions beyond faf92c2 build table qualification paths,
initialize qualified subobjects, start destructor bodies, collect reverse member/
base destruction and share the existing base-access context word.

Fresh acceptance at the access setter first failed: the native continuation
reached the same emit_virtual_base_initializers trap but had symbol index 210
instead of 211 and 333 pending expression bytes instead of 343. The earlier
begin_function_definition body incorrectly tested ClassInfo.alignment_flags
(+50) for virtual inheritance. Original instruction 0x33c89 tests flags (+51),
bit 0x10, before adding $initVBases. Source now uses that exact field; both
original/native builds assert both offsets. Related alignment-field consumers
were inspected: this constructor bit was the incorrect access.

The failed buffers, states, visible files, captures, timing components and raw
instruction evidence remain in evidence/virtual-base-hidden-parameter-failure.
The same frozen original reference was reused because functions.json was
unchanged. Corrected native buffers are 343/120/9/27 bytes and symbol index 211,
all raw bytes and all 14 state observations exact. Both cached native cases pass
in the corrected acceptance (0.411257 seconds); its 2.597743-second operation
excludes reused original capture time, which is charged only to the failed
attempt. Full 668-function canonical graph, data/default/callback and grammar
audits pass in 4.866045 seconds. The unchanged validator's 25 negative checks
last passed at faf92c2; they were not repeated for this source-only correction.

Accepted extent union 214300/476418 bytes is 44.981508%, not branch coverage.
The corrected branch is within a previously accepted function and adds no
extent bytes. virtual_diamond remains incomplete at
emit_virtual_base_initializers 0x36400.

Virtual-base guard checkpoint: 763 accepted frontiers, 678 source functions.
Ten whole functions beyond cbe46ec emit the guarded virtual-base constructor
sequence, dispatch scalar/user-defined/class-pointer conversions, serialize
short-circuit branches, initialize virtual-base table pointers, form subobject
pointers, recognize this references and compute complete/inner type sizes.
Unseen conversion/access/temporary-cleanup callees remain named trapping stubs.
The table-pointer wrapper passes both selection arguments as one. The size
routine preserves the distinct nested-array multiplication/overflow branches.

Every 0753 through 0762 acceptance matches fresh original/native traps, four
pending buffers, fourteen state fields, raw streams and visible files. The
completed cpp_basic output remains byte-exact. Full 678-function canonical
call-graph, contextual data/default/callback and grammar audits, both cached
native cases and 25 boundary rejection checks pass in 4.848492 seconds.
Individual build/reference/debug/fixture timings remain in each frontier;
a no-op preparatory build is retained separately and adds no acceptance.

Accepted extent union 216355/476418 bytes is 45.412852%, not branch coverage.
virtual_diamond remains incomplete at adjust_virtual_member_object 0x314f0.

Virtual-base pointer/copy checkpoint: 773 accepted frontiers, 687 source
functions. Nine whole application helpers beyond 5ab2866 recover virtual-base
member adjustment, checked group metadata, expression repeatability, direct
vbtable displacement expressions, deep expression copies, hash lookup and
virtual-base access. Expression-copy arena choices and owned operand/string/
floating/reference-list payload branches remain within their original function;
newly unseen payload helpers have typed named trapping stubs. ExpressionReference
and MemberReferenceGroup each have asserted eight-byte layouts.

The access wrapper first exposed the original CRT memcpy/memmove shared body.
The native source already calls memmove for overlapping inline-vector insertion.
The original LIBC MEMCPY.OBJ and MEMMOVE.OBJ have identical 334-byte text and all
13 relocation targets. The boundary proof restores that exact original body,
observes an eight-byte suffix shift by four bytes, preserves both raw pointer
words on each side, compares their Left/Right class identities and requires the
returned destination. Native GDB observes the actual memmove IFUNC implementation
entry and return to add_inline_pointer. Pointer addresses are retained raw and
compared by these two established class identities across processes.

After this one ordered CRT entry, both sides reach clear_most_derived_argument
with all four pending buffers, fourteen state fields, streams and visible files
exact. The expanded identity checker now verifies internal CRT text relocations
without suppressing existing named static CRT function edges. Existing fflush
proofs remain unchanged. Its initial finalization failure and a stale offline
cache attempt are documented; captures were reused without duplicate acceptance.
Failed preliminary source builds are also retained with separate timings.

Full 687-function canonical graph, contextual data/default/callback and grammar
audits, both cached native cases and 30 rejection checks pass in 5.283339 seconds.
Five new rejections cover library/alias identity, overlap displacement, returned
destination and lost input bytes. Build, fresh reference, boundary setup/execution,
native debugger and fixture setup/execution/oracle timings remain separately
recorded; recovery operation wall is explicitly unmeasured.

Accepted extent union 218041/476418 bytes is 45.766743%, not branch coverage.
The 334 CRT bytes are separate from application source coverage. virtual_diamond
remains incomplete at clear_most_derived_argument 0x3b040.

Vtordisp checkpoint: 783 accepted frontiers, 697 source functions. Ten whole
functions beyond 42c2207 clear the base-subobject constructor's most-derived
argument, construct the runtime/fixed vtordisp calculation and storage path,
compare pointer keys, decode class-type relations, select class bindings and
reject member-pointer arithmetic operands independently.

The vtordisp storage path retains each inherited table hop, copies the original
52-byte pointer-member descriptor into expression arena two, then assigns the
negative integer-slot displacement and integer type. All field offsets and
original helper boundaries remain intact. Newly encountered callees stay named
traps until their own paired acceptance.

All 0773 through 0782 acceptances match fresh original/native traps, four pending
buffers, fourteen state fields, streams and visible files. Completed cpp_basic
remains byte-exact. Full canonical graph/data/default/callback/grammar audits,
both cached native cases and 30 boundary rejection checks pass in 4.940909
seconds. Per-function build/reference/debug/fixture timings remain separate.

Accepted extent union 218684/476418 bytes is 45.901708%, not branch coverage.
virtual_diamond remains incomplete at convert_derived_initializer 0x662e0.

Class conversion/destructor checkpoint: 792 accepted frontiers, 706 source
functions. Nine whole helpers beyond 4273101 recover derived initializer/class
pointer conversions, fixed pointer adjustment and cast construction, destructor
call/name/argument construction and the complete operator-overload dispatcher.
Unseen reference/member-pointer and operator-search helpers remain named traps.

Class-pointer conversion preserves its interned-name comparison, pointer/cv
rules, both inheritance search directions, and rejection of reverse virtual-base
conversion. Destructor construction consumes bit 0x100 from the high parser-flag
word (source mask 0x01000000), retains the deleting-form single-evaluation guard,
and keeps shared lookup state. Deleting arguments retain the original discarded
unsigned constant before the actual argument allocation. Operator postfix
fallback warnings include the class-name argument shown by the original raw
stack setup, which the decompiler view omitted. Selected raw instruction and
global-identity evidence is in the checkpoint branch-evidence JSON.

All 0783 through 0791 acceptances match fresh original/native traps, four pending
buffers, fourteen state fields, streams and visible files. Completed cpp_basic
remains byte-exact. Full canonical graph/data/default/callback/grammar audits,
both cached native cases and 30 boundary rejection checks pass in 5.088871
seconds; individual build/reference/debug/fixture timings remain separate.

Accepted extent union 220725/476418 bytes is 46.330113%, not branch coverage.
virtual_diamond remains incomplete at new_operator_search 0x76b70.
