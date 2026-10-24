# C2.EXE reconstruction

The first `/?` diagnostic case is complete: exit 1, empty stdout and exactly
51 stderr bytes, with 19 application entries through exit cleanup. Its frozen
reference remains the native regression case. Root authorized the next case,
`numeric-control-flow`, compiled by original CL with `/O2 /G5`. Its fresh C2
argv/environment, four IL files and standalone 2347-byte COFF master are in
`cases/numeric-control-flow/`; only the COFF header timestamp is normalized.

Current reconstruction has 79 application functions (17459 bytes) and five CRT
helpers (438 bytes). The numeric case has decoded all global/initializer IL,
initialized ten sections and seven function declarations or definitions, and
entered the COFF output pipeline. The output allocator has returned through the table constructor; execution
currently stops at the COFF section allocation call with.drectve,flags0xa00,
first stackflag1 and reuse0. Four import thunks (24 bytes) and contextual text tables
(725 bytes) remain separate. The 17897 source bytes are 3.122% of 573183 `.text`
bytes; that denominator includes CRT/data/padding and is not non-CRT coverage.
`cases/first-case-result.json` describes the finished first
case, and `cases/acceptance/` retains each subsequent promotion in order.

`make -j4 native` builds i386 Linux with shared shims; `make -j4 original`
checks the Win32 reconstruction with the shipped compiler and `/MD` runtime.
`make audit` uses the common Rust CFG/RET/ownership/extent tool with explicit
paths. `make -j4 fixtures` runs only the native executable against frozen streams and
19 original application entries. Current measurements: 2.7 ms execution, 9.2 ms
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
