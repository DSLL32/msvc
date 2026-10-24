# CL.EXE 5.10 reconstruction

The `/?` case is complete: 50 reconstructed application functions, 8,175 original
extent bytes (42.89% of the 19,062-byte `.text`). The separately reviewed application extent denominator is now 18,545 bytes;
this historical help milestone retains its original whole-section percentage. The 15 startup/import support records restore another
476 bytes and receive no application source credit.

The native-only fixture takes about 28 ms including an incremental build, with
about 8 ms in the executable. It compares all 50 application entries, 3,976 stdout
bytes, 141 stderr bytes, full DWORD status0, and the strict temporary-name call's
input/output/alias/error state. Standard streams use regular files, matching the
original capture. See `cases/help/{completed,coverage,timings}.json` and the
acceptance journal for the measured evidence.

Help, usage, response/options, the genuine minimal-C compile and an intentional
unknown-option diagnostic and public preprocessor/pass controls are complete.
There are now 107 accepted application functions /18,189 original extent bytes
(95.42% of whole `.text`, 98.08% of the 18,545-byte application extent).
There are 107/112 application functions accepted, with five functions/356 bytes
remaining. `cases/code-partition.json` and its independent review classify all
19,062 bytes without overlaps: 18,545 application, 476 support, 41 literalCC padding.
These are accepted function extents, not branch or input-domain coverage. The response-file/options/help case adds five driver functions with 56 actual
entries, its own cached streams/input/trace and a ~31ms native fixture. The compile executes 75
distinct application entries, produces the exact 275-byte object and `test.c`
line, then removes all temporary IR and exits0. Source still compiles with
original CL; native dependency providers validate the real C1 DLL and C2 child
inputs before applying their captured outputs and observable CRT/OS effects.
Fresh GDB proofs cover aliases, tables, stream ownership, environment lifetimes,
seven actual unlink calls and the qualified exit call. Cached fixtures use no
GDB or Wine. Both cases take about 1.42 seconds total with a warm build; the
minimal executable takes about 64 ms and help about 4 ms, with setup/checks split in
`cases/minimal-c/timings.json`. The owned original response is under
`cases/minimal-c/reference/`, and the full final proof is archived separately.
Minimal-C has a separately captured errno0/doserrno0/LastError0 entry profile.
Its source bytes, arguments and entry state are part of every comparison.

The `/Bt` existing-object LINK timing case is complete. It matches 72 actual
application entries, two `_ftime` calls, the exact timing diagnostic, one strict
LINK invocation, its 1,024-byte output and response cleanup. The timer helpers
add 152 original application bytes. Fresh candidate proofs retain full typed
GDB clock/child/FILE/error/FP state; the cached native fixture takes about 150 ms,
including 16 ms in CL. Its immutable raw-derived packet is bound to the original
receipt and 14 frozen decoder source files. Regressions check the actual complete
child input/effects, two-call clock inventory, status, streams and entry trace;
they never regenerate that packet or decode the exhaustive original capture.
The prior 1.65-second fixture and identical executable timings are retained in
`cases/link-timing/timings.json`. All 13 cached cases pass in 3.27 seconds.

The whole timer/caller proof records that timezone-name labels are unconsumed;
original Greenwich labels remain retained without equality to native PST/PDT.
Architectural x87 fields are strict, with only the observed inert Win32/Linux
CS/DS representation qualified under empty tags, zero pointers and no metadata
consumer. Baseline analysis, failed UTC0 input, initial start-only frontier and
final normal completion remain separate owned evidence; no baseline or cached
fixture can serve as fresh acceptance. Original-toolchain and native builds
pass with `make -j4`.

The unknown-option case reconstructs warning, variadic message output and fatal
diagnostics. It matches 56 application entries, 139 stderr bytes and the intended
full DWORD status 2. Thirteen focused controls reject wrong expectations,
duplicate completion markers and status 258 with the same Unix low byte.
Its Linux fixture takes about 31 ms, including 8 ms execution; all five fixtures
take 1.75 seconds. New evidence is ordinary files under
`cases/unknown-option/{reference,completion,completion-current-audit}/`, with
hash references to existing repository originals and pinned auditor sources.
The previous evidence archives remain unchanged. Fifteen separately selected
CRT/import-support records cover 476 bytes and receive no application credit.

The public `/E /Bp1.pre /?` case enables the two frontends and changes C1's
optional leading argument file to `.pre`. A read-only snapshot at the real
help entry compares all five pass records and their bounded strings/aliases;
38 mutation controls pass. The native fixture takes about 30 ms, and all six
cached cases take 1.84 seconds. Explicit C/C++/object input wrappers and their
shared filename helper and linker forwarding are also complete. Their separate
case exits via help before executing a compiler or linker. Shared revision
`094d818f` supplies the strictly guarded CP0 ASCII `_mbsstr` call reached by
multiple-input output naming; no target workaround is used. All seven cached
native fixtures pass in 1.95 seconds. The forwarding fixture takes about 29 ms,
including 4 ms execution. Earlier unsupported-API denial evidence is retained
separately and receives no acceptance credit.

The `/D_MT /MD /TC /TP /?` case now verifies option-conflict reporting,
including the actual originating switch and replacement. It matches 54 entries,
3,976 stdout bytes and 58 stderr bytes at status 0; 16 formal/origin/alias and
four resource controls pass. All eight cached Linux fixtures take 2.02 seconds.
The individual conflict fixture takes 29 ms, including 8 ms execution.

The `/Og /O1 /?` case reaches the real option-copy path through a recognized
optimization default. Its 33-byte helper retains the originating rule, and
12 altered-input controls pass. The case matches 52 entries and completes
help at status 0. Its fixture takes 31 ms, with 8 ms execution.

An existing-object LINK case has accepted the linker-command builder and
response-file writer, with matching named frontiers and actual driver inputs.
The completed case produces the exact 1,024-byte executable from the owned
275-byte object and 82-byte response file. Its final 76-byte output helper is
accepted with the qualified cached `_write` imports and actual FILE argument.
The Linux-only child provider verifies the full command, ordered OS namespace,
LINK/CRT/PDB identities and complete inputs before applying the captured result.
Read-only GDB proves CP0/tables, environment and FILE/stdhandle identities stay
unchanged, with errors 0/2/0 becoming 0/0/0 through one actual unlink and exit.
Forty-nine native state/effect controls pass. The fixture takes 532 ms including
setup/checks and 16.1 ms in the executable; all ten cached cases take 2.78 seconds.
Shared revision `51d053557` supplies the narrow CP0 RSP2 profile. New proof is
ordinary files and path/hash references under `cases/link-object/completion/`;
earlier prefixes, raw failures and archives remain unchanged.

The `/Bz /LD /Fmcustom.map /Feout/path.dll` existing-object dry run adds the
81-byte path-stem and 185-byte map-name helpers. It emits the actual
`/implib:out/path.lib` and `/map:custom.map` options while `/Bz` skips child
execution. Six formal-input and twelve map-input/global controls pass; the
fixture compares 72 entries, 199 stdout bytes and status 0 in 35 ms (4 ms in
the executable). All eleven cached fixtures take 2.85 seconds. The earlier
help-only literal-percent probe reached neither helper and receives no credit.

The two-record `/Bz /ZM /c test.c test.c` dry run adds the 23-byte job-list
reversal: frontend commands use temporary prefixes a0/a1, then backend commands
use a1/a0. It reaches the contextual "Generating Code..." message (resource333),
now embedded in the original build and native provider. Ten job-input/link,
six proved filesystem-transport and four resource controls pass. No compiler
is invoked. Actual missing C: searches are reproduced only from this case's
hash-bound mode4/error2 observations; native fixtures do not inspect Wine.
The fixture matches 75 entries, 792 stdout bytes and status0 in 93 ms (8 ms
execution); all twelve native cases take 3.06 seconds.

The first extension lookup exposed an incorrect provisional table left unvisited
by help. The corrected ten extension/category/pass-list records and terminator,
including shared pass-list identities, and all five compiler-pass records now
match original, rebuilt and native images. A bounded initializer review found no
other discrepancy. See `cases/minimal-c/{input-kinds,initializer-review}.json`.
The verified VC5 CRT profile supplies the provider's static empty stdin buffer.
Read-only boundary captures compare FILE ownership, CRT descriptors and separate
Win32 standard handles, destinations, positions and errors through `_dup2(1,2)`.
Untouched buffer bytes are unclaimed. See `cases/minimal-c/stdio-profile.json`
and `scripts/check-streams.py` for the eleven semantic mutation controls.
The `/Gm`-disabled path makes zero MSPDB calls: fourteen original callsite guards and
a native host counter enforce this. Eight qualified stdcall declarations retain
their verified DLL exports; the native adapter always fails if invoked. The
original build uses a declaration-only LLVM import library, with decorated names
preserved verbatim and accepted by original LINK. No MSPDB behavior is supplied.

Fresh trials compare candidate-inclusive masked originals against native i386
`-O0 -g` builds: named frontier, decoded parameters/driver state, streams and
first-entry trace. The shared Rust tool audits graph/ABI/references/ownership and
extents, with narrowly verified import leaves separated from strict RET checks.
All 174 structured option rules, their terminator and five real callbacks, plus
31 driver settings and the driver-member storage alias, are compared across
original, rebuilt Win32 and native images. Compiler-generated
text tables have explicit bounded metadata. Native source builds use immutable
content-addressed provenance and a pinned shared-library revision.
The captured help entry profile restores errno0/doserrno0/LastError2 in the
uninstrumented native host; GDB verifies these at actual `main`. Unknown profiles
and argument cases fail. `cases/help/entry-profile.json` binds the raw evidence.

Source retains the known `main` name. An uninstrumented Linux host unit uses
`--wrap=main` to call it with the verified CRT setup; old `cl_main` captures are
preserved unchanged with an explicit alias in `functions.json`. Source still
builds with original CL `/MD`; contextual encountered messages are embedded by
the original RC and prepared for native use from the same payloads.

`make -j4 native native-trace` builds Linux binaries. `original` explicitly uses
Wine; `audit` is separate. `check` tests masking, current named traps, acceptance
negatives and grammar controls. `fixtures` runs all twelve completed cases directly on Linux with cached references,
shared PID-owned status/API receipts, and no Windows child or Wine. Native-only corrections explicitly reuse unchanged original captures;
failed captures and old decoder/build receipts remain intact. Earlier groundwork
below is historical; accepted references use the shipped canonical MSVCRT.

Target: `toolchains/5.10/bin/CL.EXE`, SHA-256
`aa15ed605515693292c539827ab1d8e08ce2ed5b4e1d9ac2c7f033903e6844dd`.
It reports compiler **11.00.7022**, despite the directory's 5.10 name.
PE32 i386, preferred base `00400000`, executable entry `004058e0`, relocations
stripped, no exports, TLS directory or delay imports. `.text` has 19,062 virtual
bytes. `cases/inventory.json` records exact sections and imported names from
USER32, KERNEL32, MSVCRT and MSPDB50. The latter is a real dependency requiring
an explicit boundary decision, especially for minimal rebuild/debug cases.

## Original probes

`cases/original-probes/manifest.json` records three successful original-only
runs in the shared prefix: `/?` returns 0 with 3,976 raw stdout bytes and 141
stderr bytes; `/nologo /Bd /c test.c` and the same with an explicit original
`/B2` both return 0. The final object is 275 bytes. Sources contain only
`int main () { return 0; }`. CL, C1.DLL and C2.EXE from this toolchain therefore
execute together. That first baseline did not test harness linking or any
reconstructed source build; the later harness build is recorded below.

The raw `/Bd` output names `R:\toolchains\5.10\bin\c1.dll` and
`R:\toolchains\5.10\bin\c2.exe`, and shows frontend/backend arguments. Its
formatting is not a substitute for an exact argv snapshot. CL selected IR paths
under `C:\users\root\AppData\Local\Temp` despite the launch's TMP/TEMP values;
these raw records have not been relabeled. Complete IR files were not preserved.
The explicit `/B2` run overwrote the first object; the manifest states that limit.

Later Wine launches stalled before startup, affecting other workers too. Our
first harness-build process was terminated individually after it was observed
stalled for at least 58 seconds; the fixed-R: retry timed out at 20 seconds.
`cases/original-probes/transport-stalls.json` records those limits. No prefix,
server, registry or mapping changes were made. Both runs were quiescent during
the orchestrator's hold.

After the orchestrator cleared the shared transport, the analysis-only helper
`scripts/host-contract-probe.c` built with the original CL/C1/C2/LINK in 1.465
seconds and ran in 1.606 seconds. That first attempt captured real invoke argv,
but revealed two incorrect measurement assumptions: C1's export module name is
`c1n32_l.dll`, and C2's argv contains only its executable. Its immutable raw record
and source version are under `cases/original-probes/host-contract-1/`.

The corrected helper built in **1.344 seconds** and captured the original minimal
compile in **1.603 seconds**, exit 0. Its immutable source, streams, event log,
raw MSC_CMD_FLAGS assignment, four IR inputs and final object are under
`cases/original-probes/host-contract-2/`. The manifest binds their hashes, helper
source/executable hash and target identities. The DLL debug event's complete
565,008-byte file-handle copy hashes exactly like `toolchains/5.10/bin/C1.DLL`;
the duplicate DLL bytes remain in scratch, with their path and digest recorded.

Observed ordering: DLL load event **22**, first CRT-entry trap **23** with
`(module=10600000h, reason=1, reserved=0)`, invoke-call event **25**, detach-entry
trap **27** with reason 0, C2 environment assignment event **30**, and C2 spawn
boundary event **32**. The DLL loaded at its preferred base `10600000h`; entry
`1061202ah` began with byte `53h`, preserved before the temporary breakpoint.
This proves the load event lets us intercept this DLL's CRT entry before its
export runs. **Nonpreferred-base relocation timing remains untested.** These
were analysis breakpoints, not code masking or acceptance.

The actual invoke has **27 argv strings and Boolean 1**. The backend call has
mode 0 and exactly one argv string, its executable; compiler options travel in
`MSC_CMD_FLAGS`, whose trailing space and terminating NUL are retained in
`msc-cmd-flags.bin`. The exact prefix is `.\a01396`. Before continuing the
original spawn, the helper copied all four matching files: `ex` 1,187 bytes,
`gl` 51, `in` 3 and `sy` 13. Original C2 then completed the 275-byte object.
No synthetic expanded argv or standalone backend replay is claimed. The full
remote environment was not enumerated; the manifest identifies the captured
MSC_CMD_FLAGS input separately.

## DLL host contract found in CL

Static evidence is the original full dispatch body `0040550d..004057b3`, unload
helper `004054ec..0040550c`, and control handler `004057c2..004057fd`.
The dispatcher and unload function are accepted; control-handler locations remain analysis
evidence until their own fresh function proofs pass.

* Dispatch tests pass flags bit 2. When set, it calls `LoadLibraryA(pass_path)`
  at `0040563d`; it resolves `_InvokeCompilerPass@12` from literal `00408524`
  and `_AbortCompilerPass@4` from `0040850c`. The pass object caches module at
  offset `14h`, invoke at `18h` and abort at `1ch`. A missing export unloads the
  newly loaded DLL and fails the pass.
* At `004056c9`, CL calls the invoke pointer with three stack arguments:
  `argc`, null-terminated `argv`, and `(pass_flags >> 3) & 1`. The decorated
  signature is `int __stdcall InvokeCompilerPass(int, char **, int)`; the callee
  removes 12 bytes. The Boolean's broader purpose needs further evidence; it is
  **not a callback pointer or table**. CL counts the vector itself immediately
  before the call.
* A nonzero result, or pass flags bit 3, causes `FreeLibrary` and clears all three
  cached fields. Successful flag-bit-3-clear calls can reuse the loaded module,
  so lifecycle and retained state must be captured rather than assumed fresh.
* The registered console-control handler calls `AbortCompilerPass(2)` for an
  active DLL pass, using the one-argument stdcall export. It is not an ordinary
  per-invocation callback table.
* When flags bit 2 is clear, the same dispatch uses
  `_spawnvp(0, pass_path, argv)` at `0040572e`: a synchronous child boundary.
  The successful minimal `/Bd` run identifies this route as C2.EXE.
* The DLL invocation has an SEH frame. The filter specially permits breakpoint
  exception `80000003h` to continue searching when the diagnostic flag is clear;
  other handled exceptions become pass failure. Original INT3 frontiers must
  remain debugger events rather than being converted to normal return values.

C1/C1XX workers are separately inventorying their actual DLL CRT entries, attach
logic, exported wrapper initialization and cleanup. Those routines belong to
those DLL targets. CL must not invent copies of them or count a host adapter as
DLL reconstruction.

## Dependency response boundary

The initial minimal-C dependency observation is recorded in
`cases/minimal-c/dependency-boundary.json`; the completed observation is in
`cases/minimal-c/dependency-boundary-2.json`. The latter records the actual
26-argument C1 invocation, return, detach/unload, both environment assignments,
four IR inputs, successful C2 return with its object, and normal CL exit.
C1 leaves shared CRT code page 1252 and the
en-US character table active after unloading; it also changes LastError2 to0.
The next missing-file lookup restores LastError2. These are required observable
effects, not target-side state overrides. The first C1 success boundary is now approved and exercised during a fresh
candidate-inclusive masked dispatcher trial. It stops at the genuine `unload_pass`
frontier. That raw capture, actual native request/effect/GDB proof, compiler-source
snapshots and independent review are archived unchanged in
`cases/minimal-c/execute-pass-proof.tar.gz`, with a member/hash index alongside it.
The C2 boundary is separately accepted with a fresh masked unload trial reaching
`cleanup_files`. Its raw original/native response, immutable build inputs and
independent review are in `cases/minimal-c/unload-pass-proof.tar.gz`; all 52
affected child/effect/GDB rejection controls pass. The unmasked analyses add no
code credit. The final exit trial then completes naturally with no trap; its
actual first-entry trace, full status, streams, object and seven removals match
natively. The regression reference owns885 immutable original payloads and
accepts timestamp normalization only for the COFF header TimeDateStamp. Cached
regression mode is explicitly rejected as fresh acceptance evidence.

`scripts/dependency_packet.py` freezes a recursive immutable raw manifest and
decodes those observations. `scripts/check-dependency-packet.py MANIFEST` runs
the native-only mutation controls. Analysis/baseline captures are explicitly
rejected by both the pairing controller and the independent acceptance checker.
The shared Rust register-import resolver checks the dispatcher's two cached
`GetProcAddress` sites using the narrow EDI witness in
`cases/minimal-c/getprocaddress-preservation.json`; the real Invoke call stays
indirect. Source is unchanged for the audit, and both SEH roots are included.

Start each DLL as statically linked i386 C in a dedicated native analysis host,
with the host outside the target graph. The host should reproduce the observed
attach/export-call/detach sequence, exact export ABI and caller inputs, including
whether multiple calls share a loaded instance. Original-toolchain builds must
still produce Win32 DLLs with their real names and ordinals. No ELF DSO loader is
needed for this first host design.

For CL-only native fixtures, capture the C1/C1XX in-process call's complete inputs,
status, streams and filesystem effects at the real export call boundary and
replay them strictly. Capture C2/LINK child boundaries likewise. The shared
module registry supplies the verified C1 identity, exact export names/ordinals and
one load/free lease. Typed host callbacks remain outside CL's function graph;
target source retains the original Win32 APIs and indirect export call.
Unknown modules, exports, calls, arguments and state fail closed. All fixture execution stays native-only;
new reference capture remains an explicit separate operation under Wine.

Original DLL masking needs a loader-aware debugger, not simply an on-disk INT3
image. Relocation entries pointing into code can overwrite trap bytes when the
preferred base is unavailable. The actual preferred-base `LOAD_DLL_DEBUG_EVENT` now demonstrably precedes
C1 CRT initialization, but nonpreferred-base relocation timing and writes must
still be verified on the actual transport. Proposed capture: identify the module
by file/hash and actual base, install a verified non-relocated entry-byte guard
at load, then snapshot relocated bytes and apply the full instruction mask at
the first entry trap before target code executes. Restore accepted ranges from
that loaded snapshot. This requires no TLS callbacks or import-cycle calls into
the target before entry. Do not assume that all relocations have
already run merely from receiving the event. Static DLL dependencies can load
before CL's EXE entry, so install module handling from process creation onward.
The minimal attach/invoke/detach sequence is now observed; no full-code masking
or nonpreferred-base test has run here.

`cases/plan.json` seeds help, minimal-C and `/O2 /G5` Win32 hello themes. Only
source/themes were copied from vc1.0; all arguments, IR, traces and expected
artifacts need fresh 5.10 capture. No vc1.0 oracle is reused.

The object-only `/Gm /Zi` case now reaches `jobs_from_deferred(NULL)` after
four genuine MSPDB calls. Its Linux host validates the complete arguments,
opaque engine/driver aliases, initialized pending record, public CRT/OS state,
streams and real descriptor/file effects before replaying each response.
Release, Close and subsequent lifetime effects remain unsupported. This is a
partial case, with no frontend or normal MRE completion credit.

The accepted signature builder (527 bytes), selector (33 bytes) and comparator
(20 bytes) have separate proofs under `cases/mre-object/milestones/`.
The latest proof observes the first qsort callback's two owned strings and
normal return; it does not claim a complete original array/comparison census.
The shared explicit510 `strcmp` contract is pinned at `5a8cb5ac92`. The first
native return already matched before that remap; no libc mismatch is claimed.
Private provider objects, CRT lock/heap/thread state and allocation tails remain
unobserved. Three native controller/checker failures are retained separately.

The comparator's original capture took 7.549 s, native GDB capture/checks 1.633 s
and audit 0.665 s. The parallel original/native build took 6.547 s; all 13 cached
Linux fixtures passed in 3.587 s. The 27 affected native rejection checks took
44.430 s once and the 31 raw-response checks 23.099 s. Unchanged transport and
protocol checks are reused with their hashes; cached regressions invoke no Wine.
