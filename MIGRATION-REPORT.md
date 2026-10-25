# Harness migration report

Status: interim, in progress, 2026-10-25. This records the migration to
`STANDARDIZATION.md`, including shared changes already published and work that
is still pending. Timings below are reported measurements from the relevant
workers, not a new benchmark campaign.

## Scope and current state

The migration replaces per-binary runners and nested case/evidence trees with
the shared native i386 runner, `binary.c`, numbered flat cases and
`capture.bin`. Original builds and graph/ABI checks remain separate. Normal
regressions use saved references and do not invoke Wine.

- **VC1.0:** C1, C1XX and C2 have migrated. C1XX has resumed reconstruction;
  C1's current case still needs the shared original-DIRECT facility. LINK,
  CVPACK, CL, RC and RCPP are migrating. Finished binaries are included.
- **VC5.10:** all nine binaries are in scope, including finished RC and CVTRES.
  C1, C1XX, CL, RCDLL and CVTRES have committed their migrations and paused.
  The other four are completing verification and shared integrations.
- **Watcom:** the coordinator has completed a read-only compatibility
  assessment. No Watcom migration, new capture or reconstruction was performed
  as part of that assessment; details are below.

Accepted application functions and original references are preserved during
migration. Unaccepted source work is saved separately and does not acquire
acceptance through the new registry. Historical CRT classifications and audit
qualifications do not count as reconstructed application code.

## Shared runner and build foundation

The accounting starts at tag `b4-test-migration`, so it includes the initial
shared runner as well as fixes found while migrating consumers.

| Area | Implemented behavior | Commits |
| --- | --- | --- |
| Runner and capture format | Rust i686 GNU runner loads application/case libraries; versioned C descriptors; uncompressed capture pack/unpack; native I/O and direct execution, first-entry comparison, narrow units, supervisor/subreaper, events, timeouts and workspace cleanup. | `7556959d0`, `00c38463d` |
| Explicit original capture | Shared original-only Win32 broker and PE masking/capture path. Regression execution remains native-only. A previously recorded frontier may become stale during explicit progress discovery; ordinary replay remains strict. | `00c38463d`, `7aef83dd5` |
| Native completion | Returning application adapters run normal exit callbacks and stream cleanup; terminal receipts preserve full DWORD status rather than trusting only POSIX's low byte. Direct exits must also publish status. | `7fbb7ab4c`, `10c01edb2` |
| Debugging and timing | Supervised GDB for the actual executor, with input orchestration still active; incremental quiet tool builds; explicit units on each duration. | `2e5b45657`, `27fbb777b`, `85c9073fb` |
| Shared Make rules | One application shared library; automatic flat-case discovery and generated targets; shared binary/case compiler flags; no per-case Makefiles or recursive case builds; readable support-object paths. | `9820c0788`, `552562511`, `4bcafcab5`, `36fac3867` |
| Original tools and runtime identification | Toolchain-name-based shared Wine launcher and original build support; shared runtime identification replaces local copies. Reviewed CRT overrides require registry/MAP/object identity rather than relaxed matching. | `8901269a5`, `e92c1fd0f`, `8bc59cc24` |
| Input-only environment | Capture environment fields describe inputs, not process environment changes as outputs. | `fd4d526e2` |
| Live-state native probes | One-shot exact-ABI entry hook runs a typed callback with genuine live owners/registers/stack, then finishes at the shared checkpoint without resuming parser mutation. Resume-capable observation is a separate pending feature. | `e311c8862` |

The shared tool includes native smoke cases for I/O, frontiers, descendants,
timeouts, events, direct execution and exit callbacks, plus original-capture,
Make and entry-hook tests. Consumer results below are additional evidence;
they do not imply that pending integrations have passed.

## Further shared changes found during migration

| Change | Why it was needed | Commit and verification |
| --- | --- | --- |
| Generated per-case check and unit targets | Let `make -j4` schedule independent native checks instead of handing the entire suite to one invocation. | `be363be2d`. C1's then-166-case check fell from 4.091 s at `-j1` to 1.129 s at `-j4`; C1XX checks also passed. |
| Grouped original EXE/MAP build outputs | Recreate a missing MAP correctly and avoid duplicate links during parallel builds/audits. | `b9247f29d`. A clean/missing-MAP C1 build passed with one linker invocation, 22.392 s. |
| Windows header dependency resolution | Find real SDK dependencies despite case differences and backslash separators; reject ambiguous host names. | `625530ee5`. CLI regression checks and real SDK scans passed. VC5.10 CL retained every old dependency and found 1,054 references instead of 66; uppercase Watcom headers are now discovered. No compiler/original run was needed. |
| Descriptor ABI 2, original symbol aliases and policy projection | Preserve distinct logical, native ELF and original MAP names; carry existing callback/import/RET policy into common audits without local adapters. | `0fb1c85aa`. Rust tests and native/Make smoke tests passed. Descriptor consumers require rebuilding; the capture format remained version 2. |
| Complete identity registry and runtime projection | Keep discovered functions, imports and CRT boundaries available by identity without inventing one-byte extents or treating every boundary as a reconstructed body. | `83b1fc4b9`. Also recognizes validated code-free COFF weak aliases and short import members; VC5.10 import-library identification passed. |
| Explicit runtime graph-evidence scope | Preserve each binary's previously reviewed CRT audit scope without promoting dormant CRT stubs merely because their classification or original extent is known. | `709574db9`, `fefa5c09e`. Focused tests passed; real LINK 840-graph and VC5.10 C1XX 94-graph audits passed. |
| Explicit accepted subsets for additional audits | Preserve CL's qualified import checks as a separate projection while its normal full graph audit remains intact. The named subset must contain nonempty, unique, known and currently accepted functions; stale names fail. | `fd5da7843`. Thirteen tests and formatting checks passed; VC5.10 CL's full 123-graph and scoped 111-function audits passed (`745c08633`). |
| Isolated Rust host allocation and default-deny child requests | Keep runner allocations out of target libc's heap; reject undeclared child requests even if the application handles the denial. Target allocation and reuse remain unchanged. | `1bcee0073`. Sixteen focused tests plus native/GDB/event smoke tests passed. Published RCPP rich replay passes in 83.450 ms and CVPACK DOS in 10.888 ms, with exact entry orders/outputs and unchanged target/capture hashes. C1XX's routine nine-case regression also passed. Captured-response routing is still pending. |
| Actual null stdin and explicitly held-open pipe input | Preserve NUL versus file/pipe behavior and allow an existing synchronized Ctrl+C case to wait for its input event without premature EOF. | `0fb1c85aa`, `709574db9`. Native smoke and a stdout-ready-to-SIGINT pipe test passed. |
| Explicit tracing for loaded application libraries | Register resolved application addresses/names with the existing instrumentation rather than assuming the application is the main ELF executable. | `00875e919`. Eleven GCC/Clang DSO checks, 18 existing trace checks and eight fork/exec checks passed. |
| Module ownership and shared trace attachment | A loaded application library and case library need explicit ownership for application data/callback addresses. Child execution must attach to existing trace state without resetting it. | `161776ca1`. Focused and existing tests passed; real LINK help and editor-bind cases passed. Application data ownership and case callback ownership remain distinct. |
| Explicit native self-command routing | Support a declared self-child invocation through the runner while retaining a fresh executable process and existing trace state. | `100852b60`. Eleven focused routing checks passed with GCC/Clang (0.0197/0.0241 s); eight existing process tests and both RSP2 suites passed. Actual LINK self-child integration still awaits runner support. |
| Scoped expected RET cleanup contracts | Retain C2's existing calling-convention checks without imposing one global cleanup rule on every function. | `4ff437ad2`. 65 tests, including a negative test where both sides have the same wrong cleanup; C2's 15 contracts and 1,212 paired graphs pass. |
| Strided callback tables and exact import-site assertions | Express VC5.10 CVPACK's existing callback layout, explicit null rows and resolver-site inventory in the common audit engine. | `8b6ea0935`. 67 tests and ten denial checks; real CVPACK 113-graph audit passed in about 0.202 s. |
| More precise graph-audit failure diagnostics | Identify the owner, side, source and target of an unknown tail-call ABI instead of reporting only a generic failure. | `df221a941`. Exposed C1XX's specific pending debug-helper boundary; error reporting only, with no audit semantic change. |
| Mask immediates are not function pointers | Exclude TEST/AND/OR/XOR mask constants from function-reference discovery, while retaining actual address-bearing MOV/PUSH/CMP and arithmetic checks. | `a7f50d042`, `433be1cd3`. Both discovery paths have regression coverage; the full C1XX audit passed. |

The runtime changes separate classification, available native body and reviewed
graph evidence. They do not add CRT reconstruction or silently waive application
graph checks. Explicitly qualified historical CRT vertices remain checked.

## Changes to lib/recon

The shared library maintainer made three bounded changes for the new loaded
application/case model:

- **Tracing loaded libraries (`00875e919`):** added explicit address/name
  registration through `ReconTraceEntry` and configure/enable/reset APIs.
  `recon-tool` still owns ELF symbol resolution and application/seed filtering.
  Protected hook visibility prevents libc's no-op hooks from preempting the
  actual instrumentation. Existing executable/environment tracing remains
  available.
- **Ownership and trace attachment (`161776ca1`):**
  `recon_trace_configure_shared` attaches validated existing RST1 seen-state
  without resetting it across synchronous exec children.
  `recon_register_native_image` explicitly registers application data/code
  ownership and case callback code ownership before setup. This is needed
  because LINK's workspace now lives in `libbinary.so`, rather than the main
  executable. Arbitrary DSOs, heap addresses and invalid role replacements are
  rejected; unset-owner legacy behavior remains unchanged.
- **Declared self-child routing (`100852b60`):**
  `recon_set_native_self_command` copies an immutable runner command and checks
  the actual runner's ELF32 identity/inode. It applies only to canonical
  self-spawn requests with the explicit self-spawn flag and affirmative RSP1
  permission. This handles `/proc/self/exe` naming `recon-tool` instead of a
  standalone reconstructed executable. No broker is distinct from permission;
  existing RSP2 response replay and unregistered spawning retain their behavior.

Verification included 20 focused attachment/ownership checks under GCC and
Clang (0.0259/0.0314 s), existing module-memory/provider/trace/process suites,
and real LINK help/editor-bind integration with its 36-byte workspace. The
self-route's eleven focused checks took 0.0197/0.0241 s under GCC/Clang.

These commits did not change reconstructed Win32 source, application function
graphs/ABIs, CRT profiles or runtime defaults. No original executable was run
for this shared-library work. Native-child/broker wiring is still a
`recon-tool` task, followed by an actual LINK self-child integration check;
passing the library tests alone does not establish that integration. The
maintainer currently has no additional shim edits or tests in flight.

The subsequent RCPP investigation considered isolating Rust host allocations
from libc versus giving target allocations a separate allocator instance.
The latter would require complete allocation-family ownership and error
handling; host-only isolation was selected. A native observation found the same flags with
different residual words producing hashes 83, 198, 155 and 3, using the one
loaded libc allocator. This confirms sensitivity to unwritten bytes, but does
not identify which earlier allocation supplied them. Nonzeroing allocation and
target-to-target reuse semantics must remain intact.

A scratch-only prototype then moved Rust allocations to per-allocation
`mmap`/`munmap`, leaving target libc allocation unchanged. The single authorized
rich replay passed its exact entry order, status and all 349,826 output bytes;
application library, case library and capture hashes were unchanged. Build time
was 24.183 s. Reported time was 366.616 ms (0.415 s wall time), but this was a
debug host build versus the published release runner's 81.031 ms: that is not
a valid allocator-overhead comparison. This prototype is not adopted. It establishes
sufficient isolation for that run, not a general guarantee about loader
allocations or unwritten bytes. No originals were run.

The adopted faster alternative is a separately prefixed
[tikv-jemallocator](https://github.com/tikv/jemallocator) for Rust-owned memory,
with background threads and `sbrk` disabled. Its i686 scratch build, symbol and
fork/reuse checks and one affected replay passed, without target changes.
That debug build reported 313.553 ms and also cannot be compared with the
published release runner. A matched optimized release build then passed at
85.349 ms (setup 12.420, execution 71.333, checks 1.595 ms), versus the earlier
release baseline's 81.031 ms. One sample each does not establish statistical
overhead. Runtime options, prefixed symbols, unchanged capture/binaries and
effective allocator optimization were checked. The reviewed change is now
published in `1bcee0073`; the actual published runner passed RCPP's unchanged
rich case in 83.450 ms (setup 16.422, execution 66.031, checks 0.997 ms).
This is a host-tool allocator change, not a target `lib/recon` allocator change.
Loader/libc allocation remains outside that isolation.

The published runner also passed CVPACK's previously padding-sensitive DOS
case: exact 115-entry order, streams and 6,864-byte output master hash, in
10.888 ms. No target rebuild, tunables, recapture or normalization was used.
The proposed CVPACK startup-reexec/allocator-profile feature is therefore
cancelled unless another concrete unresolved case demonstrates a need. C1XX's
routine nine cached cases and 16 unit groups also passed with this exact runner;
they were not rerun solely for the publication.

## Shared work still pending

These are approved or under design, not completed integrations:

| Work | Consumers and purpose |
| --- | --- |
| Original DIRECT helper | Generic original/native completion and masked-frontier controls passed with exact three-entry order. C1 project integration, build rules and publication remain incomplete; original-process work is paused after repeated automatic cybersecurity-risk terminations of the tool worker. No actual C1 trial has run. |
| Captured child request/response broker and native-child mode | CL, RC, RCPP, LINK and CVPACK need existing dependency boundaries replayed only after validating the invocation and complete relevant inputs. Default-deny is now published; typed captured responses and self-child runner integration remain pending. Fixtures must never fall back to Wine. |
| Post-exit output checks | LINK and RC need narrow checks after the application and its descendants have exited, including structural output checks and assertions on identified PID fields. Raw captured streams must remain intact; generic equality still applies. |
| Capture-owned harness assets outside application cwd | Keep captured dependency responses and host configuration out of exact application input/output inventories. These belong inside the capture but outside the logical application working directory. |
| CL entry-state observer and exact read-only tree membership | Preserve three existing internal-state cases and reject added SDK files, rather than validating only the hashes of known files. Concrete APIs remain to be settled. |
| Remaining graph adapters | RCPP/CVPACK registry consumers, identity-only names for extentless entries, and descriptor-aware import-thunk normalization passed six affected profiles in a frozen staged build; publication/review is pending. These must retain existing graph/ABI policy without granting extents, body availability or reconstruction credit. RC's separate support-extent guard remains pending. |

## Retired local checks and historical evidence

Migration removes duplicated runners, launchers, PE/CFG parsers and evidence
campaigns. It retains meaningful native cases, narrow behavior tests, source
layout assertions and required ABI/graph policy.

LINK's old 514-initializer campaign is historical validation of unchanged
source; it is not claimed as recurring parity supplied by the common graph
audit. Its useful contextual metadata can remain. VC5.10 CVTRES completed one
bounded migration comparison of 24 class bytes, six relocations and associated
control facts, then retired its local parser (`6bed7f7f6`). Its nine I/O cases,
depth unit, original build and shared audit passed. Neither requires a new generic
initializer framework.

The old `decomp/vc1.0/fixtures/` directory remains until every live consumer has
transferred its required inputs and references and passed the shared runner.
Existing history and scratch snapshots preserve unfinished work and removed
historical material.

The stricter canonical registry also exposed a VC5.10 C2 source identity issue:
17 distinct callback slots used a separate trapping placeholder for an original
entry already represented by a real CRT function. A shared per-owner alias
waiver was rejected. A bounded repair to use the canonical function pointer,
with consumer inventory and existing strict checks, is authorized but not yet
verified. It adds no reconstruction credit.

## Watcom compatibility report

The native half can reuse the shared i386 application/case libraries, typed
entry hooks, symbol-based tracing, full-DWORD terminal status and parallel case
execution. Native tests use the explicitly authorized GCC ABI; original Watcom
builds still preserve and audit the original ABI.

| Binary | Accepted application reconstruction | Current paused state |
| --- | --- | --- |
| WCC386 | 0 functions | Main stub; cached help output, no accepted application entry order. |
| WPP386 | 0 functions | Main stub; cached help output. |
| WASM | 0 functions | Guarded patched-help capture. |
| WLIB | 2 functions / 143 bytes | `InitMsg` is unaccepted WIP; fixtures explicitly HOLD. |
| WLINK | 1 function / 232 bytes | Cached native prefix; `Spawn` remains pending. |

Already available support includes the shared Watcom original launcher,
hash-pinned GS-patched WLINK, original build command overrides, Watcom MAP
parsing and CF graph analysis. Header dependency scanning now resolves actual
uppercase SDK names and backslash includes (`625530ee5`), with ambiguity
rejection. The recon library assessment found no concrete additional native
PIC/shim gap.

Remaining Watcom accommodations are:

1. Preserve CF payload/image/loader/relocation metadata and base-zero payload
   offsets in registry projection; do not overwrite these with PE assumptions.
2. Identify runtime boundaries in rebuilt Watcom Win32 PEs using OMF
   libraries/objects and sections such as `BEGTEXT`. A speculative whole-CF CRT
   recognizer is not required.
3. Integrate existing CF allocation, relocation, gateway and entry-control
   machinery with shared original capture. Do not treat CF as PE or bulk-mask
   mixed code/data.
4. Use typed original-compiler glue for Watcom register arguments
   (EAX/EDX/EBX/ECX) in fresh original DIRECT probes.
5. Retain GS-patch provenance, selector/owner checks, verified bootstrap
   boundaries and actual pipe/NUL semantics.

The Watcom transport milestone remains eight authored controls passed. A
thread-ordering correction was built but has not been rerun; the ninth control
must not be reported as passed. Existing FILE, nonlocal-jump and signal
limitations remain explicit. No new Watcom cases or autonomous reconstruction
are started by this report.
