# LINK 5.10 reconstruction

Target: `toolchains/5.10/bin/LINK.EXE`, PE32 i386, image base `0x400000`,
entry RVA `0x65280`. There are no exports or TLS directory. Root approved the scaffold and exact
shipped CRT startup boundary. Thirty-eight application functions are accepted,
including the complete option dispatcher and switch parser. The first `/?` case
now reaches `duplicate_argument("?")` with an independently owned parsed record,
matching context/hash/arena state and 90 semantic rejection controls. Common
Rust checks all 147 cached plus 50 direct dispatcher import calls against 197
rebuilt calls, and all five cached parser calls. Six CRT preservation contracts
serve the dispatcher; the parser additionally needs `strchr` and one reviewed
fatal-diagnostic callee. Unknown calls remain barriers. Hello and DLL cases are
queued after help; debug/PDB and resource themes remain source-only.
Native i386 and original 5.10 `/MD /Od /Z7` builds pass with `make -j4`.

`make inspect` prints the read-only PE inventory. `make native` builds the same
source using shared shims; `make original` checks the real Win32 build and builds
the original-only debugger. `make audit` uses the shared Rust CFG tool with
explicit target/index paths and RET, function-reference, ownership and whole
extent checks. `fixtures`/`check` fail closed until the first case completes.
Source-only cases
in `cases/plan.json` copy the vc1.0 GUI hello, stdcall DLL and debug DLL themes.
They contain no copied output, trace, object, reference hash or accepted frontier.

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
Unknown state contracts fail closed. Completion is also fail closed until its
full output/status/entry comparator exists.

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
evidence. Resource conversion probing awaits the RC worker's original `.res`
capture and shared Wine recovery.

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
source content and candidate-inclusive Rust provenance. No native fixture is
claimed complete yet.

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

The genuine thiscall linker context and symbol lookup require two C++ caller translation units.
It remains POD, with no constructors, destructors, RTTI, exception machinery or
C++ runtime. `RECON_THISCALL` retains native i386 ECX-this/stack4/RET4; the
original VC5 compiler supplies its implicit member convention; symbol lookup has
two stack arguments and RET8. Both structures retain C-compatible POD layouts. Other sources and
the final shared shim compilation remain C. Native compilation needs no host
C++ standard library.

The `libpath` predicate uses the distinct canonical `_mbsnbicmp` import. Actual
original call/return evidence proves CP0, C locale, one-byte characters and
normalized `-1` for `"?"`/`"libpath"` at byte limit7. The shared implementation
retains that reviewed ASCII scope; `_mbspbrk` remains typed and unsupported on
the unvisited wildcard path. `cases/help/multibyte/` is explicitly original-only
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
symbols (including genuine member names), and separately checks full DWORD1100,
POSIX76, raw streams and first-seen entry order against a completed cached
original. It refuses success until help is accepted and that complete reference
exists. Setup, execution and checking times will be reported separately.
