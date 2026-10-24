# C2.EXE reconstruction

The first `/?` diagnostic case is complete: exit 1, empty stdout and exactly
51 stderr bytes, with 19 application entries through exit cleanup. Its frozen
reference remains the native regression case. Root authorized the next case,
`numeric-control-flow`, compiled by original CL with `/O2 /G5`. Its fresh C2
argv/environment, four IL files and standalone 2347-byte COFF master are in
`cases/numeric-control-flow/`; only the COFF header timestamp is normalized.

Current reconstruction has 104 application functions (21350 bytes) and five CRT
helpers (438 bytes). The numeric case has decoded all global/initializer IL,
initialized ten sections and seven function declarations or definitions, and
entered the COFF output pipeline. The output allocator has returned through the table constructor; execution
currently stops at new_register_set(1), after four register-block constructors for selected _mix_unsigned
and the 38-byte directive payload/counters matched. Four import thunks (24 bytes) and contextual text tables
(812 bytes) remain separate. The 21788 source bytes are 3.801% of 573183 `.text`
bytes; that denominator includes CRT/data/padding and is not non-CRT coverage.
`cases/first-case-result.json` describes the finished first
case, and `cases/acceptance/` retains each subsequent promotion in order.

`make -j4 native` builds i386 Linux with shared shims; `make -j4 original`
checks the Win32 reconstruction with the shipped compiler and `/MD` runtime.
`make audit` uses the common Rust CFG/RET/ownership/extent tool with explicit
paths. `make -j4 fixtures` runs only the native executable against frozen streams and
19 original application entries. Current measurements: 2.7 ms execution, 14.8 ms
including fixture setup/checks. New capture helpers retain their actual executable
and immutable compiled inputs; newer captures also retain ELF/PE/MAP and
static-index snapshots in scratch for independent review. Earlier raw evidence
is never relabeled.
Original captures are explicit scripts and never fixture dependencies.

The initial full-text INT3 frontier was matched at `0x452797`, with the actual
loaded shipped MSVCRT file verified by hash. The whole startup, including its
cold SEH chunks, passed the common Rust graph audit and fresh original/native
state comparison. The orchestrator reviewed the candidate-inclusive protocol,
immutable compiled-source snapshots, raw GDB/ELF INT3 proof and 24 rejection
controls before promotion. Evidence is under `cases/acceptance/0001-startup/`.

## Target and dependencies

`toolchains/5.10/bin/C2.EXE` is a 660240-byte i386 PE32 console executable,
SHA-256 `e75aecaf4073b0817ffb638cae5fff636b2e2d1a090daabf8f68dbc954515fae`.
Its image base is `0x400000`, entry RVA `0x52797` (VA `0x452797`). It has no
exports, TLS directory, delay imports or base relocations. Its PE linker version
is 5.10; its embedded compiler banner says version 11.00. The binary version
must not be inferred from the folder name alone.

[cases/inventory.json](cases/inventory.json) lists the sections, data directories
and all import names. Direct imports are:

- `MSVCRT.dll`: runtime calls and data, including `_iob`, `_pctype`,
  `__mb_cur_max`, `__unDName`, `_fullpath`, `_controlfp` and ordinary stdio.
- `MSPDB50.dll`: 16 ILStore/ILMod, PDB and Stream APIs. The DLL is shipped;
  imported does not establish that an API executes in a no-debug case.
- `USER32.dll`: `LoadStringA`, for module-backed resource strings.
- `KERNEL32.dll`: `GetModuleHandleA` and `SetConsoleCtrlHandler`.

The toolchain includes `CL.EXE`, C1/C1XX DLL frontends, headers, `LIBC.LIB`,
`MSVCRT.LIB` and `MSPDB50.DLL`. Original compiler execution/actual frontend
inputs are being investigated with the CL/C1 workers. Startup source and named child stubs compile with the original toolchain;
application and CRT acceptance are tracked separately in `functions.json`.

## Proposed cases and actual-input capture

[cases/plan.json](cases/plan.json) records six source-only seeds copied byte for
byte from the corresponding vc1.0 case sources: minimal C, Win32 hello with
`/O2 /G5`, loops, register pressure, aliasing and floating point. It records the
origin and source hash. No vc1.0 intermediate file, object, trace or expected
result has been adopted as a vc5.10 oracle.

The original-only CL debugger capture in
`../cl/cases/original-probes/host-contract-2/` preserves all four intermediate
files before C2 starts, its working directory, raw `MSC_CMD_FLAGS` assignment and
the completed 275-byte object. The actual process argv contains only argv[0];
options printed by CL's `/Bd` are carried in `MSC_CMD_FLAGS`, including its final
space. `cases/plan.json` links the complete captured inputs and exact interface.
Standalone original C2 replay from these inputs remains unverified and must be
proved before treating this as an independently replayable backend case.

The original `/?` argument returns status 1, empty stdout, and 51 stderr bytes: `fatal error C1007: unrecognized flag '/?' in 'p2'` followed
by CRLF. It does not provide a help screen. Raw bytes and timing/hash receipt
are under `cases/original-help/` for the historical groundwork probe. The
canonical shipped-CRT capture now has a controlled 16-string environment, exact
argv, and streams under `cases/unsupported-help-diagnostic/`. The controlled environment also supplies the matching partial startup proof.

## Native host and outstanding questions

Use an i386 native executable with original-style Win32 C and `<recon.h>` last,
shared shim sources at final link, `-O0 -g`, and `make -j4`. C2 remains an EXE;
its inputs come from the captured original CL/frontend pipeline.
Preserve each recovered function ABI and distinguish import/runtime entries
from application graph entries. Imported MSVCRT startup or PDB APIs are not
application reconstruction coverage.

The shared maintainer supplies startup CRT contracts and a prepared module-resource
provider for encountered `LoadStringA` strings. MSPDB50/ILStore is not implemented. The reached canonical FILE layout, character flags, mutable environment
aliases and `_errno`/`strtol` coupling have fresh matching evidence. Native builds
select `RECON_CRT_PROFILE_MSVCRT_510` for the verified shipped CRT. Additional
stdio, locale or PDB contracts still require reached evidence. These are investigation items, not requests
for speculative compatibility code. Debug/PDB cases may need additional
shared support after a minimal no-debug standalone case is captured.

Source entry names are `main` and `mainCRTStartup`. Explicit driver `symbol`
aliases preserve earlier logical graph/trace names without modifying historical
captures. Only target native translation units alias `main` to `c2_main`; the
Linux host retains its own `main`. See `cases/analysis/naming-aliases.json`.

Unchanged protocol mutation controls may reuse a reviewed immutable baseline;
new or affected state checks still run with every reconstruction. The intrinsic
initializer checkpoint ran17 fresh controls and reused60 unchanged ones in
5.415s; its receipt lists both proof inputs and checker/contract hashes. The
previous complete74-control store checkpoint took21.022s.

The section-default checkpoint compares all five typed86-byte nodes, pool/hash/list
aliases, scalar fields, untouched bytes and CONST name/class address sharing.
The context at original49e300 is still UNMODELED; an approved incomplete-type
address anchor has no payload/extent/data credit and must be replaced before
any pointee use. Full107-control checkpoint time is recorded separately from
build, audit and original/native execution in its acceptance timing receipt.

The symbol-input checkpoint recovers variant fields, the64-byte transient record,
2058-byte name scratch, and the fifth source-name allocation pool. It reuses
existing option identities for constant-string blocks and frame-pointer flags.
The next unsigned reader has no formal parameters, verified from the actual
stopped ELF's DWARF; its caller state and complete transient storage match the
fresh original. The strict common audit includes all50 source vertices. This
checkpoint ran34 fresh rejection controls and reused87 reviewed controls; its
receipt also records the earlier33-control run separately.

The globals-complete checkpoint compares all4096 bytes of the target-initialized
arena with only typed pointer fields normalized, plus all1024 hashslots,
ten sections, seven correctly sized function records, pending links and both
FILE records. Proof0063 binds the accepted pool allocator's explicit header
writes and4088-byte zero writer; no malloc slack defaults are assumed.

The11-byte `read_initializers` tail wrapper is accepted with its original FILE*
fastcall child boundary. Proof0064 preserves the original partial pending index
and derives a separately reviewed complete ownership index without changing
mask, controller, source or executable inputs. The child remains INT3 and has
no reconstruction credit. Its cold chunk is126 bytes, primary163 bytes and
external dispatch data49 bytes. All104 current protocol rejection controls and
26 focused forbidden-delta controls pass; timings are recorded separately.

The complete289-byte initializer reader now passes a single full65-source
CFG/RET/import/ownership/extent audit. Resolver enablement applies only to this
owner: one EBP-cached canonical `free` call matches the rebuilt direct IAT call.
Thirteen named callee-preservation witnesses have individual bit ablations;
the canonical public CRT boundary remains external. Its28-byte root record
uses a signed DWORD offset at12, not a WORD. The post-reader proof compares all
32 FILE bytes with only base/ptr identity normalized, EOF and active-main
aliases, and the15 actually initialized buffer bytes; unused buffer tails
receive no state credit. Proof0065 records37 fresh/85 reused protocol controls,
ten provider-evidence controls and the separately timed static ABI controls.

The1894-byte `compile_functions` controller is accepted as source66, with
1497 hot bytes and397 cold bytes. Its historical logical scaffold name is an
explicit alias; source and Win32 MAP use `compile_functions`. Its64 unreconstructed
pass bodies remain typed INT3 stubs and receive no coverage. The fresh frontier
is `initialize_backend`, after the expression/symbol IL files open: exact FILE
slots4/5, reuse of the closed globals slot, full32-byte states, and existing
globals/arena/hash/queue match. Proof0066 records41 fresh rejection controls
(16.163s) and90 explicitly reused controls. Original capture is2.341s total,
native GDB capture0.494s; the separate cached native help fixture remains~2ms.

Correction0001 removes an invented duplicate pointer for the actual490b78
listing integer, and restores the cold section selector's actual495f1c
`current_symbol` argument. Historical zero-path evidence remains unchanged.
A separate actual-C condition check exercises flags0/1/-1 and rejects split
storage and wrong-owner variants; exact PE/MAP/selector/fastcall identities and
a fresh current-case pair received independent orchestrator review. No extra
function or byte credit was assigned.

The30-byte backend setup wrapper is accepted as source67. Its source argument
shares the existing `-f` destination, and its opaque record result retains the
original ECX/EAX transfer and tail ABI. The final child remains application
code, masked and unaccepted; its complete49-byte extent supplies only RET0
evidence. The new frontier is `initialize_pass_functions`, with unchanged
complete state. Proof0067 records6 fresh/125 reused controls (2.936s), original
capture2.090s and native GDB capture0.525s. Source input manifests now retain
ordinary owned files, checked byte-for-byte, with no new compressed bundles;
historical evidence is unchanged.

The583-byte output callback initializer is accepted as source68. All20 slots
are modeled as four-byte stores-only identities, with23 distinct target stubs
that remain masked, unaccepted and ABI-unresolved. Actual selected pointers
match both PE/MAP and ELF/PIE symbols. Function-reference and accepted-extent
checks now run alongside the existing complete graph/RET/import audit. The
finite callback source guard rejects invocation/escape until a real callable
signature is recovered; all20 direct slot-call attempts fail compilation.
Independent review and13 focused callback rejection controls passed. The
separate protocol checkpoint refreshed8 controls and reused123 unchanged ones.
Proof0068 records the build, capture, audit and rejection times separately.

The94-byte default-runtime-library getter is accepted as source69. It reuses
the six existing mode flags and preserves original priority, returning a static
library name orNULL. Its fresh typed boundary proves ECXkind4, EDX`LIBC`,
EAX return-pointer lineage, both actual GDB formal types, and the native
caller-local alias. Proof0069 records12 fresh/129 reused protocol controls;
the unchanged13 callback controls and20 compile guards are reused explicitly.

The56-byte directive append function is accepted as source70. Its12-byte typed
nodes retain borrowed string pointers; pool1 contains `LIBC` then `OLDNAMES` in
actual insertion order. The fresh boundary checks both nodes, exact original
string identities, the head/cursor/remaining aliases, and all4096 defined arena
bytes. Both header DWORDs and payload zeroing come from the accepted allocator;
the append function does not invent an extra write to the last link. Its full
body returns the new node, refining the earlier pending stub's unobserved result.
Proof0070 records27 fresh/123 reused controls (19.969s), refreshed20 compile
guards (2.701s), full70 audit0.482s, original capture2.326s, and native GDB0.523s.
Current validator source is retained as ordinary checked files; no new archives.

The561-byte object-output initializer is accepted as source71. It opens the
actual outputFILE, allocates the encountered pool7, and constructs the typed
32-byte writer. A fresh pair proves the actual writer formal/caller alias,
source copy, all initialized4096 arena bytes, canonical FILEslot6 and empty
partial object. Its three stored callback targets remain signature-pending
masked INT3 functions with no body/ABI credit; finite member/target escape
checks and compiled type guards hold before any callable consumer.

The full71 graph/import audit resolves both EDI-cached fopen sites using eight
individually ablated normal-return guarantees while preserving exactly the
prior13 EBP guarantees. The unexecuted MSPDB cold path is identified through
actual shipped exports/wrappers and a pinned later Microsoft public declaration;
its reviewed cdecl/thiscall boundary supplies only EDI preservation, with no
virtual implementation or executed-dependency claim. Source and dependency
witnesses, review limits,24 affected writer controls and8 ablations are in0071.
The protocol checkpoint refreshes37 controls and reuses113; its exact20 checker
inputs now use ordinary owned files and a hash manifest. All historical archives
remain untouched. Timing distinguishes the25.331s protocol controls,15.362s
writer controls and0.777s ablations from capture/build/audit execution.

The386-byte COFF initializer is accepted as source72. Its fresh pair passes
actual ushort capacity300 to the next table constructor and compares all20
copied/default globals, source/FILE aliases, and the initialized writer arena.
The allocator callback now has its byte-proven unsigned-ECX/pointer-EAX fastcall
signature; its separate12-byte body remains a masked named trap with zero
credit. Error/clock callbacks and their copied globals retain stores-only guards.
Five independently ablated ESI contracts resolve both cached malloc calls while
keeping prior13 EBP/eight EDI claims unchanged. Proof0072 records21 fresh/132
reused protocol controls (10.643s),34 boundary controls (25.924s), and5 ESI
ablations (0.515s) separately from original2.885s/native0.539s capture.
The historical free-identity negative now selects its reader owner explicitly
and asserts a real mutation; the prior no-op test failure remains preserved.
A later fixture-only shared rebuild is retained separately; promotion binds the
reviewed ELF and its exact immutable compiled-input snapshot.

Source73 reconstructs the81-byte paged pointer-table constructor. Its32-byte
layout is justified by actual append/index/iterator consumers; the one-element
page tail keeps the exact capacity*4+8 allocation. Fresh first-call evidence
checks unsigned allocator bytes32, caller ushort capacity300, both actual GDB
formal inventories, writer aliases and original return address453d8b. The
allocator itself still traps. Ten new controls passed in11.474s; the protocol
refresh passed4 controls/reused149 in4.862s, with39 unchanged prior focused
controls retained by exact hashes. Timing and all source snapshots are in0073.

Source74 reconstructs the12-byte allocator adapter as ordinary typed fastcall
C calling pool7. Its original tail and rebuilt call pass strict wholefunction
RET/graph audits. The fresh pair stops at COFF list initialization after building
the32-byte table,1208-byte page and16-byte opaque storage allocation at exact
arena offsets64/96/1304, cursor1320 and remaining2776. Full4096 arena bytes
retain allocator header/zero-writer provenance, including padding/unused tails;
no opaque-storage layout is claimed. The separate malloc4096 result is checked
by identity/role and original return-global lineage, with its payload unread and
unmodeled. Fifteen affected controls passed in15.765s;7 refreshed/146 reused
generic controls took7.957s. Native help remains19 entries/51 stderr/status1
(2.277ms execution,14.674ms including setup/checks). Full timing is in0074.

Source75 reconstructs the33-byte six-DWORD list-state reset. Exact original
and rebuilt writer sites bind each named global. In a separate auxiliary
native code test, all six start with distinct nonzero values and reset tozero,
while an unrelated sentinel survives (0.136s). Natural acceptance independently
observes kind1 and NULL second argument at the pending list constructor, with
both actual formals; these historical zero-only pointer labels will remain
unchanged as later consumer evidence refines that second value to a count-only
integer flag. Twelve new input/reset controls passed in12.916s and4 refreshed/
149 reused protocol controls in4.903s. No free-list payload layout is invented;
incomplete pointer types and unused499690's DWORD writer stay narrowly scoped.
Full timing and native fixture evidence are in0075.

Source76 reconstructs the52-byte list constructor and its24-byte layout. Actual
DWORD0/4 are buffered/count-only flags: four direct callers produce0/1, and
the fullDWORD consumer equality bypasses storage for count-only1. Conventional
Win32 int preserves the copied bits; signedness is otherwise unobservable.
Earlier zero-only pointer labels remain immutable. The accepted caller's NULL
to0 spelling has a whole-byte equivalence certificate, projecting only exact
MAP/IAT/known-source-literal/named-call addresses. A fresh pair checks both
actual integer caller formals and its allocator-zeroed24-byte record at offset
1320, cursor1344, before the pending chunk child returns. Only the encountered
chunk next-pointer prefix is typed; its complete extent remains unclaimed.
Eleven affected controls passed in12.358s;18 refreshed/135 reused protocol
controls took8.772s. Exact compiled snapshots, code certificate and elapsed
accounting are in0076; native help execution remains2.638ms.

Source77 reconstructs the34-byte hot/cold chunk allocator. The free-list path
pops only the encountered next-pointer prefix; the current path allocates508
bytes. Fresh evidence proves the completed24-byte list and self-linked chunk,
all4096 allocator-defined arena bytes, and actual char-pointer source formal
with its global/pool alias. Other504 chunk bytes retain allocator-zeroing
provenance without payload/layout credit; the separate malloc buffer stays
unread. Fourteen focused controls and the refreshed/reused protocol controls
pass with separate timings in0077. At that checkpoint a temporary shared capture flock serialized original
debugger execution; source80 removes it after the actual-hFile reader rollout.

Source78 reconstructs the222-byte source-file symbol initializer, including
both cold incremental paths. The encountered records are12-byte tagged nodes
and8-byte filename/link payloads; normal/incremental symbol writers and slot
reservation remain distinct typed masked traps. Original strlen is compiled
as the observed intrinsic. A fresh pair proves the actual record pointer and
caller filename formals, full initialized arena state, and source aliases before
the first symbol insertion. Fifteen focused controls passed in16.107s;7 refreshed/
146 reused protocol controls took7.767s. Original execution2.445s follows a
separately measured0.000067s shared-lock wait; native capture and build/audit costs
are separate in0078.

Source79 reconstructs the61-byte symbol-table insertion, preserving its
separate grow-buffer trap. The fresh pair proves only the three initialized
COFF pointer entries; remaining4084 malloc bytes stay unread/unmodeled. Full
state now includes19 symbols/12 sections and the two actual new86-byte nodes.
Original497d6c is refined to an incomplete current-section pointer, distinct
from497d4c total and497d54 index; old zero-only scalar labels stay immutable.
A whole accepted-caller byte certificate proves unchanged zero-store behavior.
An independent native actual-fastcall test starts with nonzero counters and
section sentinel, proves returned index/slot/counts and untouched section.
Sixteen focused controls pass in16.781s; refreshed/reused protocol accounting
and the0.148s auxiliary test are separate in0079. The actual loaded CRT dump
now uses explicit same-hFile offsets with exact GetFileSize/count/full canonical
SHA checks; no reopening or skipped bytes.

Source80 reconstructs the27-byte directive dispatch loop. Only callback
slot49971c gains the actual fastcall directive-pointer signature; its shared
noargRET0 alternative uses the specifically reviewed period-C pointer cast.
The other19 slots remain guarded, all23 callback targets still named masked
INT3 functions with zero body credit, and all17 shared-noop aliases retain
their original identities. No wrapper or new target vertex is introduced.
Fresh actual call-site/entry observations prove ECX, exact qualified target,
return address, ESP delta and unchanged nonvolatiles in both builds. The next
trap has the actual typed directive formal, kind4/valueLIBC and owned
LIBC->OLDNAMES chain. Canonical Win32 code independently loadsECX and calls
the exact MAP slot. Twenty-three focused controls passed in17.580s;20 compile
guards took2.815s, separately from8 fresh/145 reused protocol checks. Exact
raw/static validation, approval scope and timing are in0080. Root released
the temporary capture lock after all shared-hFile readers were corrected;
future captures run concurrently with explicit-offset/full-size/hash guards.

Source81 reconstructs the136-byte directive handler, now named
emit_output_directive with its historical logical callback ID retained by
explicit actual MAP/native aliases. Its ten-row40-byte text dispatch table
is separately owned/data-credited. A finite compiled-data certificate checks
every row, prefix/banner literal, quote flag, actual payload+8 load, scalar
writer and named child call; common Rust still owns all graph/RET/extent
discovery. The string-writer tail callee has exact60-byte/RET0 evidence only
for pending ABI transport; it stays an application masked trap with zero
body credit. The fresh frontier checks all four actual writer formals,
-defaultlib:/LIBC, NULL suffix, quote0, and borrowed caller payload identity.
Eleven input controls took13.169s;12 switch/boundary controls took0.587s;6fresh/
147reused protocol checks took6.839s. An initially irrelevant unusedEAX
negative was corrected to actualEDX, with the failed source retained. Full
build/capture/audit/fixture accounting is in0081.

Source82 reconstructs the213-byte hot/cold directive writer. It preserves
opening-quote ordering, optional suffix and trailing-space byte accounting.
The existing signed symbol offset12 now also has its observed unsigned
section-size view, with unchanged50-byte symbol/86-byte section layout. A
fresh pair reaches the same typed C2Section*ECX selector, proving the owned
.drectve86-byte record, directive-global alias and shared stored target address.
The one initializer pointer cast preserves that address only; erased slot497ce4
stays stores-only, its19 indirect callers pending, and the target remains a
real named masked INT3. All3 direct callers and the complete57-byte callee
ABI were inspected. Twenty focused controls pass in13.176s;6fresh/147reused
protocol checks take6.914s. Original/native/helper builds take6.516/2.782/1.663s;
captures2.795/.597s, audit.795s, native help1.983ms execution. The orchestrator
review and exact source/raw/elapsed evidence are in0082.

Source83 reconstructs the57-byte section selector as the same original
callback identity, now independently whole-function audited. Its section
index retains the actual DWORD initialization test at offset70 and WORD API
narrowing, while the future creator remains a separate masked typed trap.
A fresh pair proves its actual C2Section*ECX argument and caller/owned-section
alias. All20 callback slot layouts remain unchanged; no call through erased
497ce4 is authorized. The native observer now verifies its exact current ELF
owner+73/FFD0 instruction and bounds, binds that proof to the receipt and
rechecks actual loaded bias/target; historical ELF references stay untouched.
Twenty-two section controls and ten observer controls pass in15.294/7.742s;
9fresh/144reused protocol controls take9.815s. Failed setup/incomplete original
observer evidence is retained, with one corrected original capture. Full
separate build/capture/audit/fixture and correction accounting is in0083.

Source84 reconstructs the974-byte hot/cold COFF section creator. Its12-row
jump table and21-byte class map add69 separately owned data bytes. Every
class row, flags assignment, name operand and the new scalar's actual
initialized1 value is checked against the real PE. Its five cached strncmp
calls use one additional finite public-CRT EBP witness: exact canonical
56-byte/no-call provider body, actual export702, shipped cdecl declaration
and qualified IAT/load/call sites. Old13EBP/eightEDI/fiveESI witnesses remain
unchanged; removing only the new witness makes strict Rust reject. No CRT
source or body credit is added. A fresh pair proves all four actual output
formals, owned.drectve name alias, flags0xa00, stackflag1 and reuse0. The
orchestrator reviewed this finite boundary. Twenty-two section,12formal,
12compiled-data and10provider/ablation controls pass, separately timed from
9fresh/144reused generic controls. Full build/capture/audit/1.951ms-native-
fixture timing and pre-execution setup corrections are retained in0084.

Source85 reconstructs the137-byte hot/cold COFF section allocator. Its eight
reuse pointers and signed initial-1 index are contextual typed storage; exact
PE virtual-tail bounds and fresh32-byte zero state are proved. The24-byte
section layout comes from the actual caller allocation/writers, while the next
append routine retains its generic C2OutputTable*/void* signature. Fresh
ECX/EDX formals, aliases, full defined arena and record flags match before
insertion; name/index writes remain after insertion. Twenty-one affected
controls take24.394s and two storage-bound controls0.005s, separately from
6fresh/147reused protocol controls7.375s. Original/native/helper builds take
5.815/2.790/1.302s; original/native captures2.793/.622s, audit.857s, native
help2.597ms execution. Exact snapshots and timings are retained in0085.

Source86 reconstructs the78-byte generic output-table append routine. Page
allocation preserves capacity*4+8 and circular insertion order; the item stays
void*, with no universal pointee layout. The fresh pair verifies count/used1,
the first page item, the caller24-byte descriptor index/name writes, and the
creator DWORD section index1. Both actual initializer formals and creator
aliases match. Twenty-one affected controls take23.720s, separately from
6fresh/147reused protocol controls7.273s. No failed capture/setup occurred.
Original/native/helper builds take1.534/2.785/1.907s; full separate capture,
audit and native fixture accounting is retained in0086.

Source87 reconstructs the453-byte hot/cold section initializer. It preserves
the reuse/new record branches, circular linking and separate symbol/storage
children. Actual fields justify a52-byte prefix of its60-byte allocation; the
20-byte gap is uninterpreted and final8 bytes remain unmodeled. Relocation
and line blocks expose only their8-byte count/self-link prefixes, retaining
actual508/308-byte allocation sizes. No unvisited payload/default credit is
claimed. The first fresh pair reaches generic table lookup at index1, with
actual ECX/EDX, initializer ushort/flags and creator aliases checked. Sixteen
affected controls take18.952s;6fresh/147reused protocol controls7.460s.
Original/native/helper builds take6.405/2.813/1.615s; full separate capture,
audit and native fixture accounting is retained in0087.

Source88 reconstructs the87-byte generic1-based table lookup and its cold internal-error path. The first pair continues through the accepted section allocator/list constructors, proving the actual60-byte record,24-byte list,508-byte chunk and508/308-byte block prefixes with exact caller/table/backlink identities. Full4096-byte comparison relies on the already proved allocator zero writer; uninterpreted gaps/tails receive no new layout/payload credit. Twenty-one affected controls take23.977s;6fresh/147reused protocol controls7.316s. Original/native/helper builds take2.185/2.037/2.252s. Exact separate capture/audit/native-fixture timings are in0088.

Source89 reconstructs the155-byte hot/cold section-symbol initializer. Its kind2 record points to actual section data; kind5 remains a separate auxiliary record. Allfive initialized symbol pointers are captured and compared as20bytes; remaining4076malloc bytes stay UNMODELED. The fresh storage frontier checks actual data/caller/table aliases and symbolindex3 before flags/storage completion. Seventeen affected controls take19.421s;6fresh/147reused protocol controls7.513s. Original/native/helper builds take6.374/2.800/1.269s, with a separately recorded2.173s helper rebuild correcting its new five-entry log before capture. The shared61155226d review published the exact already-consumed crt.c ee55 bytes, releasing the hold without rebuild/recapture. Exact separate timings and immutable proof are in0089.

Source90 reconstructs the267-byte hot/cold storage insertion routine, including sorted incremental insertion and vacant-entry reuse. Actual16-byte header/8-byte node layout, two counters, returned index1, caller flags11 and section count1 match at the next real C2Section* recording trap. An initial validator-only formal projection still expected the prior data pointer; exact state otherwise matched. The narrow correction rejects obsolete formals in allthree projections, preserves failedchecker/diff/raw hashes, and changes no target/helper/decoder/execution input. Twenty affected controls take22.894s;6fresh/147reused protocol controls7.384s. Original/native/helper builds take6.215/2.800/1.304s. Full separate capture/audit/native fixture timing and correction evidence are in0090.

Source91 reconstructs the45-byte section hash recorder, its actual16-pointer bucket array and8-byte link. Complete array ownership, serial151/hash7, actual selector ushortCX1 and live selector-index/section aliases match after creator return. Static review caught initial link.next-before-bucket C ordering before acceptance; final C, emitted Win32/native stores and new final fresh pair preserve original bucket-before-link.next order. The initial source/raw pair remains unaccepted and separately accounted. Eighteen affected controls take20.805s; four PE initialization/bound controls0.015s;6fresh/147reused protocol controls7.519s. Exact separate initial/final build/capture/audit/native fixture timings and finite store-byte evidence are in0091.

Source92 reconstructs the103-byte hot/cold section selector, retaining missing/inactive defaults and the previous ushort result. A fresh position-query frontier proves selected index1, current section1884 and its output list1968, plus actual ushort/caller/global aliases. Thirteen affected controls take15.412s;6fresh/147reused protocol controls7.324s. The unchanged pointer-array predicate retains the four previously verified bound controls, while the new actual-PE positive is still checked. Original/native/helper builds take2.197/2.038/1.771s; exact separate capture/audit/native fixture timings are in0092.

Source93 reconstructs the159-byte hot/cold position query, preserving sentinel/current-section identity checks, missing/inactive returns and the base-offset read after the child call. Its genuine named child receives the actual C2OutputList* with selected-list and parent entry/data aliases. Both completed/used counters are compared individually in the full24-byte list and defined arena. Fifteen affected controls take17.895s;6fresh/147reused protocol controls7.385s. Original/native/helper builds take6.853/2.815/1.805s. No failed capture/setup; exact separate capture/audit/native fixture timings are in0093.

Source94 reconstructs the nine-byte output-list position query. The fresh actual pair reaches write_output_bytes with the complete twelve-byte -defaultlib: input and the same live directive caller aliases; all94 strict source graphs and14 fresh input mutation controls pass. Six fresh protocol controls pass and147 unchanged controls are reused under their recorded premise. The original/native builds, captures, graph audit, rejection costs and native diagnostic fixture remain separately accounted.

Source95 reconstructs the332-byte hot/cold byte writer with count-only, directFILE and500-byte circular-chunk paths. The fresh pair completes two real writes and matches16-byte -defaultlib:LIBC payload, used16/write_count2/section_size16 and actual uchar-space next formal. Error callback now has genuine three-argument fastcall/RET4 ABI with all19 actual slot callers and emitted original/Win32/native proof; target remains masked namedINT3, no body credit, and the cold error path is unclaimed. Clock remains stores-only. Parent independently approved the finite boundary. Twenty-six affected controls take19.935s;6fresh/147reused protocol controls7.342s. The original observer initially failed its old symbol-count conditional at the new trap; raw failure and corrected fresh capture remain distinct. Full separate timings are in0095.

Source96 reconstructs the21-byte uchar wrapper using ordinary address-taken C and the accepted byte writer. Both library directives complete: exact38-byte -defaultlib:LIBC -defaultlib:OLDNAMES payload with spaces, used38/write_count6/section_size38. The fresh next frontier has actual source char* and original497dc4/globalalias, full source string and live initialize_backend caller; returned directiveframes are not asserted. Eighteen affected controls take20.929s;6fresh/147reused protocol controls7.322s. Original/native/helper builds2.244/2.027/2.250s plus2.165s corrected-observer rebuild. Earlier successful execution raw retained with its invalid oldno-formals validator premise and wrongobserver-global label; corrected helper has its own fresh original. Full separate timings and correction scope are in0096.

Source97 reconstructs the78-byte hot/cold assembly-source initializer in assembly-output.c. Actual assembly-enabled/mixed flags normalize a four-byte active flag; the cold comment-character/header children remain named maskedINT3 with actual uchar/cdecl and sourcechar*/fastcall signatures, zero bodycredit. The fresh pair reaches record preparation with actualNULL C2Function* ECX, active0/comment0, unchanged full38B output and all defined globals. Fourteen affected controls take16.621s;6fresh/147reused protocol controls7.405s. An explicitly auxiliary native controlled7-to0 reset check takes0.155s and adds no original/case coverage. Original/native/helper builds6.499/2.792/1.346s; no failed execution/setup. Exact separate timings are in0097.

Source98 reconstructs the503-byte typed record walker and51-byte structured dispatch data, counted separately. C2Function remains56B with flags52; newly encountered flow8/debugscope32/linebase36 replace only proved holes. Actual returned list is an8B C2DebugLink chain, refining a provisional pointer return/process input without prior payloadcredit. Fresh noarg global-record helper frontier matches real parentNULLfunction/owner context and unchanged complete output/global state. Thirteen affected controls take15.869s;12 finite data/ownership controls0.417s, plus a separately recorded current-checker rerun after unused-import cleanup. Six fresh/147reused protocol controls7.573s, with23 complete owned checker sources. Original/native/helper builds6.421/2.820/2.218s; no failed executions. Data ownership/content and all98 wholefunction strict graphs pass. Exact timings and both finite-control receipts are in0098.

The global initializer preparer is reconstructed: two typed passes, exact 36-byte switch/classification data ownership and the reviewed direct physical empty-function call. The numeric case reaches process_records(C2DebugLink *NULL); pending children and seventeen callback aliases retain their guards. Source99 adds 559 application bytes; its empty-body call adds no extra credit.

At source99, byte accounting summed the exact common-audit owned code ranges: 20,451 application bytes and 438 historical CRT bytes. This corrects a six-byte understatement in the previous running tally; it adds no reconstruction credit and leaves historical reports unchanged.

Source100 reconstructs the 49-byte record dispatcher with five distinct named helper calls. The fresh pair reaches align_pending_debug_symbols(C2DebugLink *NULL), with actual ECX/global-root and caller ESI/records formals checked. The current zero flag and list root are bound to the genuine original/rebuilt slots, including emitted native PIC loads; no allocated node payload is claimed. Fifteen new controls pass, with unchanged source99 no-op/table predicates reused under their exact receipts and current positive checks.

Source101 reconstructs the 126-byte hot/cold alignment helper, using the actual signed C2Symbol.value at offset16. The common graph audit caught and rejected an initial wrong known-symbol helper identity before reference capture; the corrected source calls the genuine pending is_known_symbol. The natural case takes the NULL-list early return and reaches write_pending_debug_symbols(void), preserving all output/global state. Non-NULL node execution is unclaimed; all126bytes and the correct child graph are audited. Eleven fresh frontier controls pass; unchanged physical-noop/table controls remain explicitly reusable.

Source102 reconstructs the 35-byte pending-symbol writer and types exactly callback slot497cec from four actual call sites and two complete target ABI analyses. Both targets remain genuine named INT3s and every known original hot/cold byte remains masked; neither executes with the current NULL queue. Parent review approves only this C2Symbol*ECX/RET0 view, preserving all other19slot identities/guards. Twenty-nine source/contract/layout/compiler controls and eleven actual frontier controls pass; the current pair reaches write_assembly_symbols(void) with unchanged full global/output state. Callback body credit remains zero.

Source103 reconstructs the whole 92-byte assembly-symbol producer, including its 78-byte cold chunk and all six ECX scope flag/EDX code-address passes. Four actual function-address identities remain named masked INT3s with explicitly unresolved callable signatures; accepting their iterator consumer is blocked until those signatures are recovered. Canonical/native emitted pointer arguments and 21 finite boundary controls pass. With assembly output disabled, all six passes are skipped; 13 actual frontier controls validate the next C2DebugLink*NULL argument, parent and real output-name identity. The failed initial observer comparison is retained: original497cac is c2_output_name, not c2_codeview_types, and only the native observer/validator was refreshed against unchanged original raw.

Source104 reconstructs the 37-byte record-initializer producer, correcting its earlier provisional codeview name through an explicit historical logical-ID/source-symbol alias. Original497cac is the output-name pointer; existing emit_initializer receives the already-modeled28B C2InitRecord pointer at symbol+50. The two110B packed variable/function views preserve all other fields and offsets. No initializer payload/default or emitter body credit is claimed. The currentNULL list skips that read/call and advances to write_assembly_records(NULL). Fifteen field/source/layout/auxiliary nonzero controls and thirteen real frontier controls pass; the auxiliary observer verifies distinct initializer identities and unrelated debug-flag sentinels without running the unaccepted child.

Source105 reconstructs the whole56B assembly-record producer, with exact serialECX lookup and returned initializer pointer forwardedEAX->ECX. Its new typed children remain named masked INT3s. The lookup prefix/successRET and existing separate extent-known internal-error helper are all masked/unaccepted; no overlapping body or helper credit is claimed. Seven finite call/source/auxiliary controls pass. After the disabled assembly path, the controller selects mix_unsigned85, sets its additional_flags28->2a, removes it from the pending root and sets the volatile pass limit128. The full initialized arena/output state and actual noarg begin_function_input/compile_functions formals match; sixteen affected selection mutations pass, with a bounded exact-premise reuse certificate after correcting non-executable helper-status prose. This reaches100 reconstructed application functions.

Source106 reconstructs the whole76B function-input setup wrapper and keeps all four noarg children as genuine masked INT3s. It reuses existing current_function and code_generation_state storage. The additional function-input flag has actual DWORD storage, but this entry clears only its low byte; explicit unsigned-char access preserves the other three bytes. Twenty-four fresh actual frontier/controller/setup-state controls pass at the first reset_register_marks child, before any setup writes. Six compiled auxiliary/operand controls verify nonzero upper-byte preservation, shared aliases, call order and optional listing path. No register-table data or child body is reconstructed. The source105100-application-function milestone also passed independent orchestrator review.

Source107 reconstructs the24B reset_register_marks loop: actual75 rows/80Bstride/flagbyte+5 clearbit0x10. Only75 initialized flags are modeled;5925 pending backing bytes receive no default/layout/data credit, remain individually untouched, and unknown fields/rowpointer escape hard-fail before acceptance. Fresh original/native stop at real noarg initialize_function_records before allocation. Nonzero pendingbytes, canaryrows, livealiases, full finite87-site owner/source-use guards and actual raw before/after mutations pass102 focused controls;6 generic controls fresh/147 reused. Historical CRT438B/import24B/textdata812B separate.

Register-table data is separately limited to75 encountered flag bytes, with5925 other backing bytes explicitly unmodeled. They are checked only against each side's own before state, never as cross-side/default equivalence. The finite table-use/source guard holds any new consumer or pointer escape until the corresponding fields are reconstructed.

Source108 reconstructs initialize_function_records (179 original bytes). The actual pool1 directory allocation is1000 pointers/4000B atoffset32; cursor4032/remaining64 and all defined4096poolbytes are checked. Fresh original/native reach genuine noarg new_register_block40c75a before constructor execution. Existing pending_debug_symbols497458 is reused. Four named constructors remain masked/zero bodycredit; caller signatures and deferred one-BYTE kind+4 writer are explicit.25 actual directory/state/model negatives26.417s,16 whole-source compiled nonzero identity/order/kind controls5.641s,6 generic fresh/147 reused8.524s. Actual frontier model path+SHA is checked when reached; dormant historical models stay failclosed. Failed build/helper/setup packets are retained.

Source109 reconstructs new_register_block:139Bhot+86Bcold=225B. Four genuine calls produce four2572B blocks (12Bheader+32x80Brows), numbers0..127, linked blocks and directory entries; the five exact4096B pool1 arenas are checked under real accepted allocator zero-writer provenance. No semantic meaning/default/layout credit for remaining row bytes. Whole common graph/RET/import/extent audit passes; original /Oi preserves intrinsic memcpy. Cold capacitygrowth remains naturally unvisited, while actual-source auxiliary capacity1->2->4/nonzero-pending-byte/unsigned-wrap tests pass13controls.70 actual production-data/raw/ELF/fastcall controls avoid repeated staticaudits,5 finite source/escape guards pass,6 generic fresh/147 reused. Fresh original/native reach genuine new_register_set(poolECX1), whose body stays masked/zero credit; original future freehead slot48e124 is analysis only, no invented native table storage.
