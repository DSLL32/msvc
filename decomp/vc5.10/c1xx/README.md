# C1XX.DLL

The first case is complete: load/CRT attach, resolve both compiler exports by
name and ordinal without calling them, then unload/CRT detach. The first real
minimal C++ compilation is now active: InvokeCompilerPass has its full wrapper,
and has progressed through all initializers and both virtual work-buffer allocations.
AbortCompilerPass remains an INT3 stub.

`make -j4 native` builds the i386 Linux static harness. `make -j4 fixtures`
replays its cached original oracle with native instrumentation and no Wine.
`make -j4 original` separately builds the real DLL with MSVC 5.10; `make audit`
compares its graph/ABI/imports using the shared Rust auditor. Audit/build targets
may use Wine explicitly; fixture dependencies do not. Fixtures inspect the
already-built Win32 DLL's exports without executing it.

Accepted code is **84 application functions (5,435 bytes)**, **two historical CRT
support functions (451 bytes)** and **two compiler/linker-generated import thunks (12 bytes)**.
The 5,898 accepted code bytes are 0.7851% of the 751,216-byte code extent; this is not
a non-CRT coverage percentage. The real minimal compilation remains unfinished,
stopping at `read_env_arguments(NULL)` after complete filename case-map initialization.
The latest normal/restore proofs, public CRT adoption, affected controls and timing
accounting are in `cases/minimal/path-fold-trial.json` and `cases/minimal/PATH-FOLD.md`.

## Target and ABI

Target `toolchains/5.10/bin/C1XX.DLL`, SHA-256
`e27df3bb9a37058c85dad3fb7d1ed30b9e52d38fb6c0f66c8fdba0a11111d3d9`.
Preferred/observed base `0x10400000`; entry RVA `0x2e3c6`; no TLS directory.
The export module name is `c1n32_l.dll`.

| Ordinal | Exact name | RVA | ABI |
| --- | --- | --- | --- |
| 1 | `_AbortCompilerPass@4` | `0x903ad` | stdcall, one DWORD, ret 4 |
| 2 | `_InvokeCompilerPass@12` | `0x384d4` | stdcall, argc/argv/BOOL, ret 12 |

For native builds, target translation units use `-Dmain=c1xx_main`; the host is
compiled separately. Source keeps canonical main and the Win32 symbol remains
_main. This explicit native symbol projection adds no target wrapper or graph edge.

CL statically pushes `(pass.flags >> 3) & 1`, argv and argc. The third argument
is a Boolean mode, not a host callback table. The real invocation is captured under
`cases/minimal/contract.json`: argc26, Boolean1, raw vectors/context and a return0.
A direct original DLL host reproduces all four CL-produced IR files byte-for-byte.
`cases/minimal/invoke-validation.json` records the wrapper promotion and11 rejected
input/state/ELF/extent controls. The initializer uses a reviewed limited typed projection: only encountered fields
are represented, with unknown original bytes explicitly unmodeled. Its concrete
implementation/proof packet passed review in `cases/minimal/initializer-review.json`;
no frontend data section has been bulk copied. Repeated invocation resets frontend state and runs `.initfe`
callbacks; cancellation, SEH, PDB/COM and resource lookup need later cases.

## First-case evidence

`cases/lifecycle/validation.json` records immutable original/native captures and
the exact trial audit. `functions.json` records each promotion in order.
`cases/lifecycle/reference.json` is the native fixture oracle; no fixture silently
captures original output. Captures preserve actual loaded target/MSVCRT bytes,
raw state and streams, relocated code snapshot, full mask readback and bindings
to the compiled native/Win32 artifacts and configuration.

The canonical shipped MSVCRT is 277,776 bytes, SHA-256
`e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210`.
Original hosts use an adjacent verified copy and `msvcrt=n`; DLLLOAD hFile bytes
prove the provider actually loaded. The entry guard is outside all 26,460 HIGHLOW
relocations. At its first trap, all 751,216 loaded code bytes are saved and masked;
only complete accepted/trial hot and cold chunks are restored from that snapshot.
No loader data exemptions or nonpreferred-base certification are claimed.

The native host registers the DLL's verified identity through `lib/recon`, calls
the same stdcall lifecycle source as the real Win32 DLL, and checks post-return
state. Attach count goes 0→1→0; the empty 128-byte onexit allocation is freed while
its end pointer intentionally retains the original dangling identity. Raw callback
and initializer slots stay NULL. The saved module is the registered DLL, thread
notifications are disabled, LastError stays 203 and caller x87 precision stays
53 bits. Host-only observation preserves LastError. The application entry trace
is the original-derived first-encounter `DllMain`; CRT/import/shim entries are
filtered on both sides.

Two-chunk CRT functions are single logical vertices in the Rust audit. Import
`_initterm` is an ordinary external declaration; LINK emits its six-byte thunk.
Separate source/import counts prevent treating that thunk as handwritten code.
The full-case acceptance rejects 20 altered completion/state/identity/mask/audit
proofs. Earlier CRT-entry and initializer gates retain their own negative controls.
Source contents are frozen by compiler wrappers. `scripts/check-history.py` and
`check-completion-negative.py --historical` revalidate the committed completion
receipt against immutable execution snapshots and a fresh static Rust audit.
They cannot promote functions, change raw receipts or execute either binary.
Current native fixture execution remains a separate check.

## Boundaries still pending

The saved module feeds LoadStringA (256-byte buffer); resources will need actual
encountered IDs/encoding/capacity behavior before native support is claimed.
MSVCRT FILE, `_mbctype`, SEH/longjmp and wide-character imports require provider
contract checks. MSPDB50 and OLE methods are in-process stateful interfaces, not
subprocess responses. The lifecycle, Invoke, typed initializer and real allocator are accepted; minimal compilation remains in progress.

`cases/plan.json` retains source-only C++ seeds copied from vc1.0; it contains no
old IR/oracle reuse. Original builds/captures use the shared prefix and fixed R:/S:
drives, under `S:\vc5.10\c1xx`, with no private mappings or server operations.

Consumer cross-references refine each context to pointer / unsigned position /
stack pointer. Schema5 auxiliary seeds use scalar17 identically on both sides.
Historical schema2–4 pointer-seeded captures establish zeroing/reset only, and
are not evidence of numeric equivalence before clearing. Raw historical captures
remain unchanged.

Eight output objects have nine encountered fields each: 264 semantic bytes,
with 24 bytes of natural native C padding. The original 259-byte gap per object
is unmodeled. Schema 13 accounts for 540 semantic BSS bytes and a native sizeof
of 564; 8,516 original initialized snapshot bytes remain unmodeled.

`cases/minimal/output01-trials.json` through `output08-trials.json` retain each
natural pair and both explicitly auxiliary nonzero constructor/reset modes.
Unused callback and owned-pointer roles are stated in each trial. The decoder
checks every constructor entry, field assignment, destructor argument and
ordered cleanup slot. Split initializer wrappers retain disjoint chunks and
separate constructor vertices. Cleanup registration preserves its cold variadic
diagnostic boundary. The eighth-object checkpoint records checker, controller
and input hashes, 20 fresh state controls and 20 fresh historical lifecycle
controls, with timings; no original lifecycle execution was repeated.

The deferred-work constructor adds one proven pointer field, with frame contents
unmodeled. Its natural and nonzero/reset proofs are in
`cases/minimal/deferred-trials.json`; schema 14 has 544 semantic BSS bytes,
24 native padding bytes and a sizeof of 568.

The callback18 sentinel-token copy passed independent review: complete original
40-byte internal transfer is checked separately from 17 modeled native bytes.
Only zero/numeric auxiliary input is validated; real pointer inputs require
identity mapping, and both 32-byte tails remain unmodeled. The proof packet and
42 rejection controls are in `cases/minimal/token-copy-trials.json`.

The signature accumulator adds a proven unsigned32 field, reset on repeated
initialization and seeded with all ones by callback21. Its three pairs and
ten signature-specific controls are in `cases/minimal/signature-trials.json`.
Current BSS projection:565semantic bytes plus27native padding, sizeof592.

At that checkpoint reconstruction had reached initializer38; minimal compilation remained
unfinished. The completed lifecycle fixture remains native-only. At the PCH
checkpoint its execution took 2.30 ms; rebuilding consumed 2.30 s separately.
The accounting is in `cases/minimal/pch-checkpoint.json`.

The ordered PCH wrapper/member closure has separate function identities under one
reviewed joint proof. Its 68-byte receiver has typed fields and opaque pointees;
prospective raw-only captures carry no graph credit. Callback38 clears two proven
queue pointers. The intervening original tail slot and pointed-to nodes remain
unmodeled. Four fresh pairs and 17 new rejection controls are recorded in
`cases/minimal/pending-group-trials.json`. Schema25 models 959 semantic BSS bytes
and 29 natural native padding bytes; 8,431 initialized bytes remain unmodeled.

Callback39 is accepted as one 24-byte split wrapper:10 primary bytes plus14
cleanup-registration bytes. Its constructor and destructor remain separate
named traps. Three fresh pairs and six changed-boundary controls are recorded
in `cases/minimal/comment-wrapper-trials.json`; no joint closure was needed.

The generated-comment constructor is accepted separately (226 bytes). Its
74 observed character bytes, COM flag and two lengths add86 semantic BSS bytes;
unknown original capacity/tails remain unmodeled. Four pairs and22 new controls
are in `cases/minimal/comment-constructor-trials.json`. Schema26 has1045 semantic
BSS bytes and31 natural native padding bytes. The next frontier is callback40.

Callback41 clears deferred-action head/tail pointers and a signed nesting depth;
node layout remains opaque. Four pairs and18 new negative controls are recorded
in `cases/minimal/action-trials.json`, with GDB observation timing separate from
fixture execution. Schema27 has1057 semantic BSS bytes and31 padding bytes.

All42 initializer callbacks are accepted; the real `main` remains a named trap.
The current frontier verifies argc26, original/native argv identities and the
Invoke mode assignment after initialization. Nine new entry/ABI controls are in
`cases/minimal/empty42-trials.json`. Minimal compilation is still unfinished.

The native GDB controller compiles one hash-checked collector body per process.
Thin checkpoint wrappers select verified output paths and execute in fresh
namespaces; every mutable field is still read at each stop. The same48-observation
case fell from12.09s to3.40s, with identical logical checks/trace/streams and eight
cache guards. `cases/minimal/collector-cache-verification.json` preserves both
immutable native receipts and the reused original. The lifecycle fixture remains
separate:12.81ms total,1.98ms execution, zero Wine invocations.

The actual main is now accepted (137 hot+27 cold bytes) and stops in its separate
work-buffer initializer. Three fresh paired runs and34 new controls verify actual
main-entry ABI, all new default/live/saved initialized fields and the isolated
phase write. Schema28 models137 initialized semantic bytes plus51 native record
padding;8383 original initialized bytes remain unmodeled. BSS is1069 semantic
bytes plus31 padding. See `cases/minimal/main-trials.json` and the main plan.

The67-byte work-buffer initializer is accepted, stopping at the real two-argument
reserve helper. Four pointer fields add16 semantic BSS bytes (1085 semantic plus31
padding, sizeof1116). Three fresh pairs and21 new controls are in
`cases/minimal/work-trials.json`. No VirtualAlloc behavior is yet accepted here.

The 92-byte split reserve/commit helper is accepted separately. Three paired runs
verify four actual VirtualAlloc calls, all formals, EDI/nonvolatile preservation,
RET16, unchanged LastError203/x87 state, owned region maps and each committed16KiB
read as zero. The reserved remainder is opaque; no64KiB alignment semantics are
claimed. Full original BSS and initialized/saved-data guards constrain the known
main continuation. The next helper's real argv formal is observed. The packet is
`cases/minimal/allocation-trials.json`:43 targeted controls plus two necessary
cached-import-contract ablations. Original captures took5.27–5.67s; native GDB
observations3.99–4.18s. These are capture costs, separate from lifecycle fixtures.

The163-byte split early-option scanner is accepted with no new data. Its first
real parser call is the captured argv[1] string "-il" and state0. The caller/argv
identity, formals and unchanged original/native state pass17 fresh controls in
`cases/minimal/early-options-trial.json`. Getenv and environment tokenization are
unexecuted beyond this frontier; tokenizer/table consumers remain pending.

The internal-switch parser is accepted after independent first-table review:
216code bytes, with its16-byte .text jump table separately owned/restored as data
and six padding bytes still masked. All25 actual argv calls/returns, three new
int fields/defaults and nonzero snapshot/restore pairs pass32 targeted controls
plus3 common Rust ownership denials. See `cases/minimal/early-switches-trials.json`.
Initialized projection is149 semantic bytes+51native padding;8371original bytes
remain unmodeled. Parser promotion adds no data bytes to code coverage.

The one-byte empty hook is accepted after independent namespace/profile review.
The host now binds the actual original CRT argument/environment vectors separately
from the 26-entry Invoke vector. Full ordered private inputs, borrowed storage
identities and the real absent-variable getter result are checked; a Linux-only
host-variable collision cannot leak into the target namespace. Earlier absence
observations did not certify that namespace and are retained with that limitation.
The actual GetSystemInfo call uses the reviewed shared RSI1 profile: all36 SDK
bytes, stack/nonvolatile/error/FPU state and neighbors match, exactly one call
is allowed, and GlobalMemoryStatus remains denied. Three fresh pairs and23 new
controls are in `cases/minimal/system-info-trials.json`. Original observations
took8.43–8.64s and native GDB observations4.50–4.65s. Completed lifecycle fixtures
remain native-only:13.03ms total,2.15ms execution. Minimal compilation continues.

The81-byte environment wrapper is accepted with its full loop/member/failure
graph, while the natural dynamic frontier is its first no-argument helper.
Nine contextual descriptors and nine typed arena objects have exact layouts,
original defaults, owned pointer identities and nonzero BSS reset proof.
The first auxiliary caught missing native field clears; the rejected pair is
preserved, and all three fresh corrected pairs pass.15 new controls take46.34s
once for this new state group; earlier unchanged protocol controls are reused.
Schema31 has1373 semantic BSS bytes+31 padding; initialized8371 bytes remain
unmodeled. `cases/minimal/environment-trials.json` records the observations,
build timings and the12.59ms native-only completed lifecycle fixture.

The76-byte default-arena selector is accepted separately. Its fresh pair reaches
CompilerArena::Reserve for arena0 with the actual descriptor0 and NULL address;
raw ECX, both stack inputs, complete member formals and actual callers match.
Eleven new affected ABI controls pass in34.65s; the unchanged nine-arena reset
campaign is referenced with its input hash. `cases/minimal/default-arena-trial.json`
records the8.58s original and4.80s native observations. No member body or return
behavior is accepted by this selector proof.

The103-byte reservation member is accepted through its first calculator call.
Its full thiscall/RET8 graph and public VirtualAlloc edge are audited; the fresh
dynamic pair proves only the descriptor-pointer write and actual next cdecl4
input/caller. Nine new affected controls pass29.24s, with unchanged prior state/
member-input controls referenced explicitly. The auditor update required only
static re-audit,0.098s, preserving both raw executions. Original/native observations
took10.84/4.95s. `cases/minimal/arena-reserve-trial.json` records that finite scope.
The same original run captured the earliest full x87 context. Its originalSW20
versus current nativeSW0 is an explicit baseline limitation; the independently
reviewed shared empty-entry profile will be applied before lifecycle when the
next calculator candidate consumes floating-point state. No reseeding at a
calculator frontier or full IP/DP/empty-register-payload equivalence is claimed.

The57-byte signed64 reservation calculator is accepted after independent review,
with its automatic6-byte public _ftol import recorded separately with zero app
credit. Three paired proofs cover the actual9 native x87 operations, live80-bit
values, QWORD conversion, truncating/control-word restoration, rounded result
and real TOP_DOWN reservation ownership/API through Reset. The once-before-DLL
empty x87 entry profile is bound to our own original context, not seeded at the
calculator.28 new FP/state/ABI controls plus8 support-scope controls pass;
`cases/minimal/scaler-trials.json` retains the exact finite first-call scope.
Captures took9.48–9.74s original and5.18–5.36s native GDB; the separate completed
lifecycle fixture took13.46ms total/1.90ms execution with no Wine.

The17-byte void Reset member is accepted after all nine real arena iterations.
Natural and both explicitly seeded/restore pairs verify each receiver, signed64
conversion, owned reserve-only API result and actual Reset entry/RET0; nonzero
cursor/limit/committed/reset-value inputs prove its four assignments and whole
neighbor guards.25 new affected controls pass; unchanged profile/protocol controls
are hash-referenced. `cases/minimal/arena-reset-trials.json` records the14.59–15.14s
original/5.30–5.55s native GDB observations and13.36ms native-only lifecycle fixture.
No new data fields, import support, reserved-payload reads or meaningful Reset
EAX return value are claimed. The next real frontier is initialize_pool_arenas.

The121-byte pool-binding wrapper is accepted with all ten calls in its audited
graph. Its natural dynamic frontier is the first BindArena call: pool10 as the
actual ECX receiver, arena index0 on the stack, complete source formals, actual
CALL/caller and unchanged target state/FP context. Eight focused ABI/state
controls take0.35s after the full paired/strict graph check; no new data or
repeated unchanged control campaign. `cases/minimal/pool-binding-wrapper-trial.json`
records16.25s original/5.38s native GDB and14.09ms native-only lifecycle fixture.
The other nine binding calls and the member body are not claimed as dynamically
executed by this wrapper proof.

The25-byte BindArena member is accepted separately: full original BSS and native
modeled/padding/saved/external guards prove only pool10.arena is assigned arena0.
The actual next Refill request1/ECX receiver/formal inventory/caller is observed;
its body and return are pending. Nine affected controls take3.12s. A fixture
auto-build encountered unrelated shared Watcom WIP, so acceptance waited for
READY, then refreshed only native evidence against the unchanged original.
Profile factories now permit exactly the native ELF binding delta; five guards
reject mask/case/rebuilt/candidate/other trial changes. Both execution/source
snapshots and trial hashes are retained. `cases/minimal/pool-bind-member-trial.json`
records15.39s original/5.54s refreshed native and13.86ms native-only lifecycle
fixture (2.10ms execution). No new data fields or support bodies are credited.

The54-byte split Refill member is accepted (47hot+7cold), after the auditor
publication hold and successful static-only re-audit of the same raw pair. Its actual request1,
SDK page_size4096 and existing minimum_pages1 produce one page, passed to the
real arena0 with a4-byte output pointer aliasing pool10.remaining (initial0).
Raw ECX/stack2, complete formals, actual CALL/caller, whole original BSS and
native modeled/padding/saved/external guards match at the named AllocatePages
frontier. Ten affected controls take3.16s; unchanged earlier state/protocol
controls remain hash-referenced. `cases/minimal/pool-refill-trial.json` records
16.05s original/5.59s native and14.04ms native-only lifecycle fixture. No new
fields/support are introduced. The cold max branch is preserved/audited, while
its dynamic outcome and the post-callee cursor/return await later execution.

The73-byte AllocatePages member is accepted to its real CommitExtent call.
The first page-count1 request writes available4096 to pool10.remaining and
advances arena0.cursor by4096, preserving every other original BSS byte and
native modeled/padding/saved/external byte. CommitExtent has a proved actual
ECX receiver, no stack arguments, complete formals and real CALL/caller; no
hidden EAX input or memory commitment is assumed. Eleven affected controls
take3.47s. Constructor/loop-prefix checks use real earlier captured checkpoints,
with current two-field writes validated separately. `cases/minimal/arena-page-trial.json`
records15.35s original/5.29s native and14.46ms native-only lifecycle fixture.
No new data or support bodies are credited; the old-cursor return awaits the
genuine commit helper returning.

The117-byte CommitExtent member is accepted (87hot+two15-byte cold chunks).
A fresh pair proves its real64KiB MEM_COMMIT/API/provider result, exact region
protections, complete zero bytes and opaque remaining reservation. All ten
pool binding/refill/allocation sequences now execute:31 method returns and64
ordered entry/return events verify ECX/stack arguments, cleanup, nonvolatile
registers, returned page pointers, FP/error state and complete neighbor guards.
The next options-processing stub observes actual argc26 and the separate Invoke
argv.21 affected controls pass in6.34s; earlier unchanged controls remain pinned.
`cases/minimal/arena-commit-trial.json` records20.76s original,5.86s refreshed
native GDB and2.41s lifecycle fixture including2.40s native rebuild (2.46ms
execution). The original was reused unchanged across the reviewed shared-source
rebuild; both immutable native executions remain available. No new fields or
shared support were needed.

The236-byte checksum initializer is accepted, adding three complete256-byte
tables and reaching the actual multibyte helper at39394. Normal and nonzero
restore/overwrite pairs match the full table outputs, callee ABI, unknown-byte
and neighbor guards. The intervening isolated `setlocale(LC_CTYPE, "")` is now
served by reviewed shared support: LCcodepage1252 and a new classification table,
with internalMBflag/codepage still0, retained oldtable, persistent returned string,
unchanged errors/FP/private vectors. Native builds explicitly select the510CRT
profile; the initially denied unprofiled attempt is retained.38 affected denials
pass. `cases/minimal/checksum-trial.json` records original24.31/25.53s and native
GDB6.03/6.97s; native-only lifecycle replay is13.60ms (2.20ms execution).
`scripts/check-checksum-history.py` statically revalidates both frozen pairs.
The checkpoint has82 application functions/5280codebytes; historical CRT451B
and generated imports12B are separate, against the751216B whole code extent.

The338-byte environment-argument parser is accepted after fresh natural and
nonzero restore proofs. Its full quote/backslash/MBCS body compiles and passes
the strict graph audit; this case executes the genuine NULL path only. Both
sides return unchanged, the caller applies ten warning defaults, and the next
real trap is `new_option_vector(40, 1)`. Sixteen affected controls reject. See
`cases/minimal/ENVIRONMENT-ARGUMENTS.md` and `env-arguments-trial.json` for exact
proofs/timings; `scripts/check-env-arguments-history.py` revalidates immutable
pairs without execution. Current85 application functions/5773B plus historical
CRT451B/imports12B total6236/751216 whole code bytes; minimal compilation remains
incomplete.

The116-byte vector allocator is accepted after independent normal/restore
review. Three real40/1 calls return12-byte headers and160-byte pointer storage
from arena1, with a genuine32KiB commitment and full zero/neighbor/ABI checks.
Normal execution stops at authenticated MSPDB50!SigForPbCb("CPP",3,ffffffff)
before provider execution; restore reaches next_argument. No provider success
or code credit is claimed. See `cases/minimal/VECTORS.md`/`vector-trial.json`.
Current86 application functions/5889B plus historical CRT451B/imports12B total
6352/751216 whole code bytes; the normal dependency still needs a guarded
captured response before the minimal C++ compile can continue.
