# LINK 5.10 reconstruction

Target: `toolchains/5.10/bin/LINK.EXE`, PE32 i386, image base `0x400000`,
entry RVA `0x65280`. There are no exports or TLS directory. The first `/?`
case is complete: 45 application functions (19,672 original extent bytes), plus
13 separately classified CRT/import support vertices (522 bytes). Five of six
CRT bodies and four of seven import leaves are reached by this case. This is
an accepted-extent inventory, not a whole-program non-CRT coverage percentage;
there is no complete instruction-only denominator yet.

Native i386 and original 5.10 `/MD /Od /Z7` builds pass with `make -j4`.
`make fixtures` runs only the native ELF against cached original evidence:
45 entries, exact 1430 stdout bytes, empty stderr, full DWORD 1100 and separate
POSIX 76. The measured runner takes 55 ms, including 4 ms of execution; the complete
incremental make invocation takes 131 ms. Detailed evidence, fresh/reused control
counts, hashes and timing are in `cases/help/completion-validation.json`.

`make inspect` prints the PE inventory. `make native` builds using shared shims;
`make original` checks the real Win32 build and builds the original-only debugger.
`make audit` retains shared Rust CFG, strict RET, qualified import, reference,
ownership and whole-extent checks. Unreconstructed calls remain named traps.
The source-only hello, DLL and debug-DLL themes in `cases/plan.json` contain no
copied vc1.0 oracle. Hello-win32 is in progress after independent first-case review: 55 application
functions (27,852 extent bytes) are accepted in the working prefix. Its filename
canonicalization and library-path search have fresh masked/native evidence. The
next frontier is `LinkContext::InitializeSections`; hello-win32 has not completed.

## Original-only probes

Explicitly run `make capture-help`, or `python3 scripts/reference.py --mode
masked` for fresh acceptance evidence. These invoke originals and are never
fixture prerequisites. The helper copies actual CREATE_PROCESS/LOAD_DLL `hFile`
contents for identity hashing. The transport verifies fixed shared R:/S: mappings
and uses fresh `S:\vc5.10\link\...` working directories. Native shipped MSVCRT,
MSPDB50, MSDIS100 and IMAGEHLP are selected explicitly.

The successful fresh original `/?` reference in `cases/help/original/` exits
with the full Win32 DWORD **1100**, writes 1430 stdout bytes and no stderr.
Its loaded MSVCRT hFile has SHA256
`e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210`.
The native POSIX projection is 76; it must be checked separately from the full
logical status, which is recorded separately by the shared terminal-status FD protocol.

The initial `/help`-equivalent `/?` probes did not reach LINK: shared-prefix
prestartup health failures also affected other tools. One timed-out run's raw
logs and receipt remain in scratch, summarized by
`cases/original-probe-incomplete.json`. Its timeout is not LINK's expected result.
Those failures remain historical transport evidence. They are not the current
help baseline. No debug DLL, PDB or child invocation has been dynamically
validated by this worker.

## Acceptance gate

`scripts/accept.py --candidate NAME --write-trial PATH` creates a trial prefix
containing the candidate. Capture both sides with that exact trial config using
`reference.py --mode masked --config PATH` and `native-frontier.py --config PATH`.
Promotion requires root scaffold approval, byte-bound config/runtime snapshots,
current binaries and source hashes, verified actual target/CRT modules, matching
raw streams, a nonempty matching frontier, a raw-derived state contract, evidence
that the candidate executed on both sides, and a fresh Rust audit of the trial.
Unknown state contracts fail closed. Normal completion requires `--complete`
and a fresh `--native-cleanup` GDB capture, checking all three constructor
allocation generations through their actual delete returns. A separate native
completion run verifies full 32-bit status, POSIX status, streams and entry order.

The initial whole-text mask traps at original entry `0x465280`; the native host
traps at the named `target_startup`. `cases/scaffold-validation.json` records
19 negative checks, including stale mutually matching snapshots, candidate graph
omission, incorrect frontiers and invalid ownership. Original process entry has
no C arguments or application state; OS-loader register values are outside that
cross-platform boundary. Later application boundaries need explicit state checks.

`cases/runtime-entry-identity.json` is static identity analysis only: anchored
CRTEXE.OBJ bytes, all relocations, contextual SEH data and reached-callee identity
candidates. Nothing in that report itself accepts or unmasks runtime code.

## Dependency findings

`cases/inventory.json` records every imported symbol. Unlike vc1.0, this LINK
imports MSVCRT, MSDIS100's C++ disassemblers, three MSPDB50 functions and four
IMAGEHLP functions, as well as KERNEL32. Static analysis also locates dynamic
MSPDB50/MSPDB41 selection and resolution of `PDBExportValidateInterface` and
`PDBOpen`. These are in-process interfaces; subprocess response replay alone
cannot implement their object ownership and state.

`cases/dependency-evidence.json` records original static call sites for the
synchronous CVPACK and CVTRES child paths. The visible CVPACK argv is `cvpack`,
`/nologo`, quoted output path; there is no `/cvnew` in this call. CVTRES receives
machine, `/nologo`, `/readonly`, optional Windows CE flags and quoted input.
Both have sibling-tool lookup and PATH fallback. These observations establish
candidate contracts, not that any seeded case reaches them. The CVPACK and
CVTRES workers were notified; actual 5.10 captures must settle full argv,
environment and file effects before building strict native child replay.

The shared recon maintainer confirmed that MSDIS100, MSPDB50 and IMAGEHLP have
no current shim implementation. No speculative implementation was requested.
Root must choose the in-process dependency policy using reached ABI/state
evidence. Resource conversion and debug/PDB dependencies remain outside the completed
help case; their contracts require actual 5.10 case evidence.

The executable `.text` section also contains IAT/import metadata and string
data (the IAT starts at RVA `0x1000`). Section executable flags do not establish
instruction ownership; future masking must preserve verified non-code ranges.
All regressions will run the native i386 reconstruction with cached original
references, never Wine. Reconstruction source remains original-style Win32 C,
includes `<recon.h>` last and must also compile with the original toolchain.

The operator-new C import spelling uses a zero-section COFF weak alias, preserving
the actual `MSVCRT.dll!??2@YAPAXI@Z` import and cdecl ABI. The shared native
provider implements the evidenced successful allocations, zero-size and oversize
contracts; heap failure and unknown new-handlers fail closed. Fresh raw
call/return observations bind allocation sizes and normalized pointer ownership.
The shipped `atonexit.obj` closure is separately byte/relocation verified; native
callback registration arguments and successful results are compared in order.
`cases/help/frontiers/` records acceptance order, raw evidence, frozen compiled
source content and candidate-inclusive Rust provenance. The completed cached
reference is under `cases/help/completed/`; fixture execution never refreshes it.

Main retains its complete ordinary and SEH paths and the five tool modes.
Its optimized original caches printf/setvbuf in EBX; debug parsing caches
isdigit/sscanf in EDI/EBP. Finite instruction certificates prove qualified IAT
identities, callsites, all register-alias writes through restores, and incoming
branch cases. Full Rust graph/RET/ref/ownership checks still include both bodies;
qualified-import checks exclude only these documented residuals, reproduction
setup, command-line parsing and switch lookup (getenv/fullpath/comparison caches). Forty
negative mutations reject wrong slots, calls, register writes, branches, restores
and dispatch-row identities. Native exceptions are still fatal.

The VC5 hardware profile is an explicit frozen sample of the original
GetSystemInfo/GlobalMemoryStatus outputs. Native captures require
`--environment-reference CAPTURE`; profile preparation validates the raw receipt
and exact call inventory without Wine. Only the encountered page-size and
physical-memory fields are consumed. Volatile memory availability is not an
invariant. The shared verified VC5 CRT profile preserves the explicit setvbuf
flag and buffer ownership; full FILE state remains checked.

The genuine thiscall linker context, symbol lookup and full-link driver require C++ caller translation units.
It remains POD, with no constructors, destructors, RTTI, exception machinery or
C++ runtime. `RECON_THISCALL` retains native i386 ECX-this/stack4/RET4; the
original VC5 compiler supplies its implicit member convention; symbol lookup has
two stack arguments and RET8. Both structures retain C-compatible POD layouts. Other sources and
the final shared shim compilation remain C. Native compilation needs no host
C++ standard library.

The `libpath` predicate uses the distinct canonical `_mbsnbicmp` import. Actual
original call/return evidence proves CP0, C locale, one-byte characters and
normalized `-1` for `"?"`/`"libpath"` at byte limit7. The shared implementation
retains that reviewed ASCII scope; The shared `_mbspbrk` now handles the reached ASCII search; wildcard expansion
remains an application trap. `cases/help/multibyte/` is explicitly original-only
analysis, separate from masked acceptance.

The three standalone output callbacks are genuine nonvirtual member pointers.
Original/Win32 globals contain one code DWORD; native globals contain code plus
zero receiver adjustment. `scripts/member_pointers.py` accepts only the six
named methods and an unambiguous null, with exact original assignment/indirect
call sequences and Win32 code stores checked. Forty-two mutations reject wrong
code, adjustments, widths, nulls and call/assignment changes. This narrow ABI
accommodation does not apply to embedded fields or serialized data; the context
remains664 bytes on both builds. The contextual128-byte DOS image is output
data, defined as a typed header, bootstrap bytes and message, and matches the
original payload exactly.

The completed constructor boundary checks both string tables, hash descriptors,
four arena descriptors and full reserved pages, directory/cell/bucket links,
auxiliary state and standalone callback identities. The first original
VirtualAlloc call is captured before/after with an initially zeroed page; native
calls are additionally checked in order against the four concrete arena roles.
No unsupported error-state or allocation-granularity equivalence is claimed.
Only the explicit pointer ownership fields and identified clocks project across
platforms. Eighty-two semantic mutations reject heap/API/configuration drift.

The prepared native help runner executes only ELF, resolves actual application
symbols (including genuine member names), and separately checks full DWORD 1100,
POSIX 76, raw streams and first-seen entry order against a completed cached
original. It requires the accepted help case and its complete cached reference. Setup,
execution and checking times are reported separately.

The hello-win32 search acceptance retains both actual ABI arguments, stdcall
stack cleanup, and the full 260-byte caller buffer before/after. An empty libpath
list returns false without writes. The subsequent `_searchenv` call binds the
actual default-C/CP0 LIB directory, filename, output prefix and preserved own
tail, plus errno/DOS/LastError 2 before and after. Its next cleanup frontier
compares the owned `nologo` copy, parser storage, input/option queues and context
heap. 67 semantic negatives and the strict 60-vertex graph pass. Fresh original
and native GDB captures took 1.320 s and 0.258 s; graph checking 0.336 s. Cached help
remains 56.3 ms total / 3.93 ms ELF. Full compiled ELF, PE and MAP bytes are now frozen
with each new native capture, in addition to source snapshots.

The 17-byte `release_memory` wrapper is accepted after observing all three
real option-copy frees and normal returns. The next frontier records its one
`LinkContext *` formal, full owned context/heap, output-name argv alias and the
reached option globals. 64 semantic negatives pass. Original MSVCRT free may
rewrite its own argument slot while releasing a small-heap block; that residual
is retained as raw evidence and is not a caller-visible preserved parameter.
A failed observer that incorrectly assumed slot preservation remains in scratch.
Native-only retries bind the unchanged original receipt/mask/inputs explicitly
when unrelated shared-header changes require a native rebuild.

The 435-byte option validator is accepted with all original branches retained.
The normal hello case disables PDB/incremental operation and reaches FullLink.
Its hidden ECX receiver and one stack out-pointer are captured explicitly;
the latter aliases the real caller local. The initial out-byte is uninitialized
and recorded on each side without cross-platform value equality. All context,
filename/option queues and option effects match, including the intentional
reset of buffer4 remaining count before full linking. 70 focused negatives pass.

FullLink preparation recovers typed section/contribution/group prefixes and
several LinkContext pointer/size roles. These preserve the existing 664-byte
context layout. The original-toolchain object retains all 3,799 code bytes and
166 effective relocation targets; compiler-local label numbering differs. A
fresh native capture matches the unchanged original FullLink boundary, with
70 semantic negatives and the strict audit passing. Unknown record tails stay
unmodeled; this preparatory layout work did not itself receive acceptance credit.

FullLink's entire 6,889-byte extent is accepted, including its cold section-merge
continuation. Its callees remain named traps. Both compiler builds pass, and the
candidate-inclusive pair reaches initialize_output_modules with its actual
output-slot argument, initial NULL, full context/heap and caller out-byte ownership.
The common audit checks 63 vertices and 56 real return ABIs. Its strict per-site
import check resolves all eight cached imports and ten direct imports; both
member callbacks remain genuinely indirect. The 32 ordinary preservation
contracts and one narrowly scoped header-callback contract are necessary under
independent removal/bit tests. A new data initializer taking the callback slot's
address is rejected before that contract can be enabled.

The original capture took 1.162 s. An unrelated shared-header update required a
native-only retry (0.287 s), bound to the exact unchanged original inputs/mask and
its retained raw receipt. Its typed-positive/build/ABI checks are fresh; the 73
unchanged semantic negatives and controller/slot/import controls are explicitly
hash-bound reused, not rerun. Final audit cost is 0.849 s total, including 0.302 s
binding/setup and 0.491 s command/check phase. Cached help takes 64.9 ms total,
including 57.4 ms setup, 3.95 ms ELF execution and 0.43 ms checking. The durable
acceptance and both independent reviews are in
cases/hello-win32/frontiers/50-full-link. Earlier pending evidence remains
unchanged; no downstream pass or hello output completion is claimed.

The 100-byte output-module initializer is accepted. It preserves the incremental
calloc path and ordinary linker-arena path, allocates complete156/68-byte records,
and delays publishing its output until both names are copied. The next actual
string-copy input is captured as `" linker defined module"`; the two reached name
literals own only their exact47 .text-data bytes. The initial masked-literal
capture remains unchanged with an explicit correction record. Both record
contents, output-slot NULL, separate arena/page ownership and all six actual
VirtualAlloc call/return inputs and initially zero pages match. 86 current semantic
negatives pass. The original capture took1.324 s, native GDB0.316 s and the full
audit0.831 s total (0.451 s command/check phase), with64 vertices/57 real RET ABIs.
Prospective FullLink counterpart addresses were refreshed without changing its
original body/provider/closed-target semantics; each prior archive retains the
actual earlier proof/checker/object bytes in full-link-proof/.

The 85-byte duplicate_link_string wrapper is accepted with both allocation paths
retained. The actual ordinary-link case observes both label inputs, normal returns
and stdcall cleanup, disjoint copied names, complete module/library records and
delayed output publication. Contribution-pool initialization is the next trap,
with no formal or hidden register inputs. Both builds, 93 fresh semantic negatives
and the strict 65-vertex/58-return audit pass. Detailed fixture and capture timing
is in cases/hello-win32/frontiers/52-duplicate-link-string/timing.json.

The 66-byte contribution-pool initializer is accepted. The first newly reached
allocator has all four actual formals captured as NULL,4000,0x3000,4, and no hidden
register inputs. Both pool pointers, capacity and ready flag remain zero at this
call. Both builds,100 fresh semantic negatives and the strict66-vertex/59-return
audit pass. Timing and exact raw evidence are under frontiers/53-contribution-pools.

The 44-byte application VirtualAlloc wrapper is accepted, retaining its normal
return and allocation-failure callee. Both actual pool calls bind all four inputs,
normal RET16 cleanup and returned owners. Complete4000/20000-byte buffers are
zero-filled, distinct from the other live allocations, with capacity500 and ready1.
All eight original/native VirtualAlloc calls are checked in order.102 fresh
semantic negatives and the strict67-vertex/60-return audit pass; timing is under
frontiers/54-virtual-memory. No new shared shim was needed.

The 11-byte COMDAT initializer is accepted as an arena-head reset. The next
section initializer has its actual hidden ECX receiver bound to the same complete
664-byte linker context and no explicit or additional hidden inputs. Both builds,
106 semantic negatives and the strict68-vertex/61-return audit pass; timing and
raw evidence are under frontiers/55-comdat-init.
