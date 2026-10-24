Proposed single workflow: debug PCH metadata and real call-owner interfaces

Read-only after dde6825fca. No capture, compiler/test execution or reconstruction
change has started. Source and ABI names below are descriptive proposals, not
recovered original symbols. Target extents remain provisional until fresh entry
and whole-body evidence. No application/CRT credit is taken by this survey.

The original keyword initializer1c330 iterates the57 actual table rows at6ab50.
None emits token129(TOK_SELF) or41(TOK_SEGMENT); no spelling contains self/seg.
Its single-underscore aliases only duplicate existing rows. The grammar retains
TOK_SELF in state233 and legacy segment productions, and based_type_modifier can
return modifier8, but that alone is not a public token producer. Existing valid
named-base declarations select modifier5, not the self bit8. convert_based_pointer
26210 calls validator26180 only when the target shape has bit8 and the source is
not the same type (after its constant/other early paths). No genuine source path
into that self shape has been established. Do not propose __based(_self) as known
supported syntax, create a synthetic token, call the modifier then inject parser
state, or call that a public source producer. Validator26180..26203(131B) stays
blocked. A real future producer or approved direct interface would be needed.

One public source/PCH workflow

Use genuine supported /Yc-/Yu flag spellings from the existing debug-PCH case,
-Z7 and browser output, a guarded metadata.h, three ordinary -D definitions and
three relative -I directories. Produce metadata.pch, then consume that exact
producer output in a second native/original phase. The header defines a record,
callback/prototype types, const/volatile and unqualified objects, and the global
prototype metadata_sink(const MetadataRecord *,unsigned,double). The consumer's
first call is metadata_sink(&metadata_seed,META_SCALE,0.5), so the named callee
and all three typed argument nodes are real. The rest of the function traverses
records through a callback and nested loops; static record/string initializers
exercise the ordinary metadata path. No self/segment syntax is mixed into it.

First freeze one successful untouched original per public phase, including all
IR/browser/PCH bytes and status/streams. Native reuse must consume its generated
PCH and match. These public phases establish owners, not promised new coverage.

All direct units borrow one genuine finish_call_arguments entry in the PCH
consumer, before that parent finishes. Its actual call parameter gives the
completed ArgumentNode chain; call_context->symbol supplies the parsed global
function symbol, and restored pch_saved_defines/include_directories supply real
permanent vectors/strings. A fixed first named call avoids conditional stepping.
Validate the actual callee/name/arity, not an assumed first arbitrary call.
All direct units terminate externally after their own observations: no transient
owner escapes or return into parser continuation, no whole PCH writer on these
argument nodes, and no fabricated compiler/token state.

Nine body candidates,696B total

| Address extent | Bytes | ABI/effect | Actual owner/context |
| --- | ---: | --- | --- |
|44320..443d3|179|RET4, one tree/argument pointer; marks type/+8 and variant payload slots|Completed three-node ArgumentNode chain at finish_call_arguments|
|446a0..446e6|70|RET4, vector pointer; converts each item through pointer_to_pch_reference, updates its cache, does NOT store items|Restored populated pch_saved_defines; snapshot every item and cache|
|449a0..449f8|88|RET4, encoded-reference vector; computes decoded locals but never stores items|A genuine new_vector/append_vector owner containing actual reference values derived by accepted pointer_to_pch_reference on the restored strings|
|44b80..44b91|17|RET8, two unsigned sizes, EAX unsigned minimum|Nested real callee of the next copy function; previous15B note omitted two RET operand bytes|
|44c00..44c36|54|RET12, source/copybytes/allocationbytes; allocate_permanent then inline min-bounded copy; EAX new owner|Live vector's used item bytes, with both truncation and larger allocation capacity; no spare source capacity read|
|4a360..4a3a5|69|cdecl two args, owned output and actual FrontendSymbol; NUL name at output+1 and byte length at output0; meaningful AX result|Actual metadata_sink symbol/name and bounded owned256B destination|
|2c420..2c447|39|RET4, actual FrontendSymbol or NULL; EAX bool(effective_type_qualifiers(symbol->type))|Two genuinely parsed qualified/unqualified objects restored from PCH|
|270e0..27122|66|cdecl0, browser include callback then record14 and browser_suppressed=1 once|Real live browser stream/template; invoke twice, second call no additional packet; end direct phase|
|2a1b0..2a222|114|cdecl variadic: initializer buffer, format, args; format into1024B local then write_initializer_bytes|A separate constructor-owned WorkingStack via initialize_work_stack, actual symbol name and vector count, bounded "%s:%d" output|

The two vector bodies exit with EAX equal to the input vector in the inspected
code, but no external caller establishes the intended source return type. Bind
that actual exit value in fresh ABI proof; do not claim a recovered prototype.
The Pascal-name helper uses movsx AX,CL; do not assert an invented full32-bit
return contract. Use an ordinary bounded actual identifier initially and record
AX, destination bytes/terminator and untouched tail. The walker/browser helpers
have no demonstrated consumed return value. All argument/pop/result claims must
be checked on fresh original/native entries and completion.

Concrete safe ArgumentNode owner

make_call_argument2bc80 allocates sizeof(ArgumentNode)=20 in compiler_arena pool2,
sets flags2, kind TOK_ARGUMENT, type and expression. append_argument2bc30..2bc71
then writes previous(+16) and circular next(+8). finish_call_arguments receives
the completed chain. For a valid declared three-parameter call, every walker-read
field (+4 type,+8 next,+12 expression,+16 previous) is defined. Ordinary
make_ir_leaf/make_typed_ir_expression leave+8 unwritten and remain invalid inputs.

Before calling44320, bind each actual allocation/block and all three20B extents;
check flags2, types, forward ring, backward chain, expected argument order and
real children. Enumerate all current permanent_pages and their actual extents,
validate page count/map ownership, and prove EVERY visited slot is outside every
permanent/PCH address range. Arena labels alone are insufficient. Under that
concrete condition accepted mark_pch_pointer returns0 for the expression slot,
so44320 follows previous links but does not recurse into an ordinary expression
with unwritten+8. If actual ranges overlap, a field is unwritten, or a variant
would recurse, stop/report rather than zero fields or repair the node. Observe
unchanged nodes/children and only the actual PCH reference/cache effects. No
relocation-bit mutation is promised for these temporary nodes.

Other direct ownership/effect checks

The saved definition/directory vectors are made by prepare_pch_output's genuine
copy_vector(...,arena0,copy_string) and restored from the real produced PCH. Check
count/capacity/used slots and each string's actual permanent page ownership.
Encoder70 leaves all slots unchanged. Decoder88 must receive only references
obtained from those accepted page mappings, stored through new_vector and
append_vector into a fresh owned vector, with each page/offset checked. Preserve
its apparent missing-store bug: assert unchanged encoded slots and no invented
restoration effect. Do not repurpose pointer-valued live compiler slots as fake
encoded input.

The permanent-copy54/min17 pair uses initialized used bytes only. Compare the
actual new allocation, copied prefix relationships and remaining allocation
bytes without initializing target memory merely to obtain a desired answer.
No byte-copier42bf0 provenance claim follows from this separate frontend helper.

For formatter114, ordinary local WorkingStack *buffer=NULL is input to the real
accepted constructor; this is owned API setup, not zeroing compiler state. Use
its initialized16B owner/2048B block, bounded actual name/count and supported s/d
format specifiers, then accepted finish/read methods to inspect only the bytes
actually written. No1024B overflow/invalid-format campaign. Browser suppression
unit uses the actual live stream and template from loaded/compiler state, checks
packet/pending data and one flag transition, and stops; no successful public
continuation after deliberately suppressing browser output is claimed.

Known reach limits / excluded work

Within known whole app bodies plus these candidates, the only direct references
to the nine are44320's recursion and44c00->44b80. No external source caller or
non-reloc absolute function pointer was established. This is direct support,
not dead-code proof and not public reach. Static absence is not CRT ownership.
A broader public-reach claim would need a real producer/caller.

2a4b0..2a4dd45B declaration diagnostic and4be50..4bed2130B packed-DOS weekday are
not added just to inflate this workflow. The former would need a justified real
diagnostic context; the latter has no demonstrated DOS-date owner in this Win32
frontend path.17040/17080 record helpers remain behind17070/17140 RET/fall-through
ownership ambiguity.42bf0 byte copier remains attribution-uncertain. No unknown
is classified as CRT and no ambiguous interval is unmasked by priority.

Read-only source provenance caution: the local PE.read helper does not virtualize
.bss when raw-file offset is0. No apparent file bytes at browser template6387c
are used as data evidence. It is in the loader-zeroed .bss section; the actual
live template/meaningful fields still require the proposed real state capture.
The raw reference scan has no positive .bss-derived claim.

If root considers this grouped metadata interface too broad, the honest smaller
core is walker179 + vector70/88 + permanent copy54/min17 =408B. Do not replace it
with a tiny self-token test or silently claim all remaining code is CRT.
