# CL.EXE 5.10 reconstruction

The `/?` case is complete: 50 reconstructed application functions, 8,175 original
extent bytes (42.89% of the 19,062-byte `.text`). A complete non-CRT denominator
is not yet classified. The 15 startup/import support records restore another
476 bytes and receive no application source credit.

The native-only fixture takes about 28 ms including an incremental build, with
about 8 ms in the executable. It compares all 50 application entries, 3,976 stdout
bytes, 141 stderr bytes, full DWORD status0, and the strict temporary-name call's
input/output/alias/error state. Standard streams use regular files, matching the
original capture. See `cases/help/{completed,coverage,timings}.json` and the
acceptance journal for the measured evidence.

Help artifacts are frozen and independently verified. The authorized next case is a
genuine minimal-C compile. Its in-process frontend and backend child boundaries
require complete captured response contracts; unknown dependencies fail.
The driver prefix has added twenty-two functions with fresh candidate-inclusive
proofs; it currently traps at `execute_pass` with the verified 26-argument C1 invocation.
Minimal-C has a separately captured errno0/doserrno0/LastError0 entry profile.
Its source bytes, arguments and entry state are part of every comparison.

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
The `/Gm`-disabled path makes zero MSPDB calls: six original callsite guards and
a native host counter enforce this. Six qualified stdcall declarations retain
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
negatives and grammar controls. `fixtures` runs the completed help case directly on Linux with cached references,
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
These addresses are analysis locations, not accepted reconstructions.

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

## Proposed workflow, awaiting approval

Start each DLL as statically linked i386 C in a dedicated native analysis host,
with the host outside the target graph. The host should reproduce the observed
attach/export-call/detach sequence, exact export ABI and caller inputs, including
whether multiple calls share a loaded instance. Original-toolchain builds must
still produce Win32 DLLs with their real names and ordinals. No ELF DSO loader is
needed for this first host design.

For CL-only native fixtures, capture the C1/C1XX in-process call's complete inputs,
status, streams and filesystem effects at the real export call boundary and
replay them strictly. Capture C2/LINK child boundaries likewise. Existing shared
`LoadLibrary` support covers kernel32 exports only; root and the recon maintainer
must choose an explicit target-module registry or a precise CL call-site seam.
Unknown modules/exports/calls must fail. All fixture execution stays native-only;
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
