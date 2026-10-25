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

- **VC1.0:** all eight binaries have committed their migrations and layout
  cleanup. C1XX and CVPACK are reconstructing; C1 has completed all identified
  application stubs and retains two unattributed routines. Finished binaries
  are included.
- **VC5.10:** all nine binaries are in scope, including finished RC and CVTRES.
  All nine have committed their migrations and paused. Their 38 invocation
  cases, original builds and applicable graph/ABI checks pass.
- **Watcom:** the coordinator has completed a read-only compatibility
  assessment. No Watcom migration, new capture or reconstruction was performed
  as part of that assessment; details are below.

The updated migration layout also separates reconstructed application code
(`src/`), justified binary-specific CRT implementations (`crt/`) and native
hosts/fixture/test glue (`harness/`). This applies to finished and already
migrated binaries; folder cleanup is now an additional migration requirement,
with all nine VC5.10 binaries now committed. VC1.0 C1, C2, CVPACK, RC and
RCPP have committed this cleanup, as have C1XX, CL and LINK.

Accepted application functions and original references are preserved during
migration, with the explicit source corrections documented below. Unaccepted
source work is saved separately and does not acquire
acceptance through the new registry. Historical CRT classifications and audit
qualifications do not count as reconstructed application code.

Completed VC5.10 migrations:

| Binary | Commit | Cases | Reported native fixture time |
| --- | --- | ---: | --- |
| C1 | `72e82f150` | 2 | 5.3–7.9 ms per case |
| C1XX | `a7233aef5` | 2 | 5.0–7.6 ms per case |
| C2 | `dccbd1906` | 2 | 7.8–8.7 ms per case |
| CL | `745c08633` | 14 | 349 ms total |
| CVPACK | `efab2086e` | 2 | 7.2–32.8 ms per case |
| LINK | `6e70c7e1d` | 2 | 5.6–18.8 ms per case |
| RCDLL | `a43df3cdc2` | 2 | 5.2–171.8 ms per case |
| RC | `783eff148` | 3 | 8.6–148.3 ms per case |
| CVTRES | `6bed7f7f6` | 9 | 55 ms total |

These measurements come from different individual verification runs, rather
than a simultaneous whole-project benchmark. Unaccepted work remains preserved
for later reconstruction.

Completed VC5.10 layout cleanup and final verification:

| Binary | Layout commit | Original build | Checks/units | Audit |
| --- | --- | ---: | ---: | ---: |
| C1 | `a756f5dcb` | 4.653 s | 0.066 s | 0.137 s |
| C1XX | `b1b39da03` | 5.081 s | 0.068 s | 0.165 s |
| C2 | `c207a8a78` | 7.482 s | 2.140 s, including native build | 0.311 s |
| CL | `4bbf117c7` | 3.931 s | 0.226 s | 0.242 s |
| CVPACK | `c3f9bee61` | 7.918 s | 0.365 s, including audit | included |
| LINK | `5a0fb06df` | 4.855 s | 0.084 s | 0.310 s |
| RC | `b179ccca2` | 1.208 s | 0.417 s | 0.133 s |
| RCDLL | `3b8e5f2fa` | 10.468 s, including native build | 0.244 s | 0.501 s |
| CVTRES | `a41ff4e76` | 4.438 s | 0.165 s | 0.150 s |

These moves preserve application bodies, ABIs, captures and reconstruction
credit. The coordinator independently reviewed all nine source layouts.

VC1.0 layout checkpoints are committed as C1 `f74f9a8cc`, C2 `344558081`,
CVPACK `c900599ec`, RC `8dc33696f` and RCPP `a6b518f4b`. Final cached check/unit
batches took 0.335 s for C2, 0.174 s for CVPACK, 0.191 s for RC and 0.328 s
for RCPP. C1's 172-case batch took 10.940 s including case-library rebuilds,
with summed setup/execution/checks of 1.801/1.121/0.671 s; units took 1.120 s.
C2's 7.752 s audit command included rebuild work and is not a pure audit timing.

RC's completed migration preserves 137 accepted application functions. All
18 cases, two rejection tests and 193 paired graphs pass. All 254 unique
accepted extents are checked on both sides; support identification provides
123 original and 118 rebuilt intervals. Five previously unaccepted runtime
declarations remain unavailable. Original compilation took 15.379 s and the
strict audit 0.593 s. The exact old 108-record original runtime catalog is
retained because registry projection alone omitted previously known callees.
No identity classification adds CRT reconstruction credit.

RCPP's completed migration is committed in `d55f26767`. All 20 native I/O
cases and the two nonempty unit groups pass, including 40,048 classifier and
131,080 catalog checks. The parallel replay/unit batch took 0.317 s; its
161-function audit took 0.237 s and original compilation 8.775 s. All 179
historical original/rebuilt extent guards remain covered. The 112 accepted
application functions and original captures are unchanged.

CVPACK's migration is committed in `b1475f9bf`: ten native I/O cases and ten
unit executors pass, with 187 accepted application functions and 20 stubs
preserved. Original compilation took 3.799 s; native rebuild and strict audit
took 1.536 s. Actual I/O fixture time totaled 208.224 ms. The final two affected
cases took 19.601/34.821 ms; their build/check batch took 0.742 s. The remaining
eight cases and units took 0.388 s including case builds. Its queued strip case remains for later reconstruction. The old conditional
CRT guards were subsequently found to be a bookkeeping restriction rather than
a requirement for new guard machinery; the reviewed correction is below. The authorized padding correction is described below.

C1's actual original-DIRECT integration and current residual case are complete
in `bd5093ae9`. The final assembly helper adds one application function / 98
bytes; exact `_strncpy` identity adds no application credit. Final original and
native DIRECT executions pass at the same entry, including ABI, streams,
workspace and status checks: 1.395 s original versus 20.227 ms native. The
combined original/native/case rebuild and strict audit took 11.334 s;
173 cached native cases took 1.478 s and unit executors 1.162 s. The accepted
application total is 983 functions / 228,861 bytes. C1 paused after that scoped commit and then resumed at the user's request,
with a substantial PDB record/arena DIRECT case.

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
| Live-state native probes | One-shot exact-ABI entry hook runs a typed callback with genuine live owners/registers/stack, then finishes at the shared checkpoint without resuming parser mutation. Resume-capable observation was subsequently added in `a4954756a`. | `e311c8862` |

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
| Isolated Rust host allocation and default-deny child requests | Keep runner allocations out of target libc's heap; reject undeclared child requests even if the application handles the denial. Target allocation and reuse remain unchanged. | `1bcee0073`. Sixteen focused tests plus native/GDB/event smoke tests passed. Published RCPP rich replay passes in 83.450 ms and CVPACK DOS in 10.888 ms, with exact entry orders/outputs and unchanged target/capture hashes. C1XX's routine nine-case regression also passed. Captured-response routing was subsequently published in `016683376`. |
| Capture-owned harness assets | Store response payloads and host configuration inside the capture but outside application cwd; expose them to case setup and units without environment metadata. | `5341dfc25`. Eighteen tests and real C accessor/tamper controls passed. Complete bytes, membership and modes are checked after descendants exit. No descriptor ABI change. |
| Typed captured child responses and post-output checks | Validate declared child requests before replaying captured status, streams and file effects. Run optional file-only checks from pristine setup state after the application and other descendants exit, preserving raw streams and generic equality checks. | `016683376`. Nineteen Rust tests, transport/lifecycle controls and native smoke passed. A focused timing control assigns the full checker phase to checks and real descendant lifetime to execution. Native self-child integration remains pending. |
| Optional export discovery without missing-symbol loader errors | Inspect actual dynamic exports before optional lookup, avoiding loader error-string allocations that polluted the target's libc heap. | `0bcd5d006`. Focused symbol visibility and E/D integration controls passed. CVPACK's erroneous type-server branch is resolved; its separate output-padding issue was addressed by the authorized source fix below. No allocator behavior changed. |
| Actual null stdin and explicitly held-open pipe input | Preserve NUL versus file/pipe behavior and allow an existing synchronized Ctrl+C case to wait for its input event without premature EOF. | `0fb1c85aa`, `709574db9`. Native smoke and a stdout-ready-to-SIGINT pipe test passed. |
| Explicit tracing for loaded application libraries | Register resolved application addresses/names with the existing instrumentation rather than assuming the application is the main ELF executable. | `00875e919`. Eleven GCC/Clang DSO checks, 18 existing trace checks and eight fork/exec checks passed. |
| Module ownership and shared trace attachment | A loaded application library and case library need explicit ownership for application data/callback addresses. Child execution must attach to existing trace state without resetting it. | `161776ca1`. Focused and existing tests passed; real LINK help and editor-bind cases passed. Application data ownership and case callback ownership remain distinct. |
| Inline C++ construction and SDK portability | Preserve proven original inline construction without adding constructor/placement helper vertices; bind allocating operator new to existing recon_cpp_new, deny unsupported pure-virtual calls, and expose SDK ERROR_DISK_FULL=112. Real App methods remain outlined and instrumented. | `477bd3d95`, `aaa964cd1`, `446d1949e`. GCC/Clang instrumented O0 placement checks pass (about 0.033 s each); pure-virtual link/normal/deny-125 checks pass (about 0.055 s each); four i386 SDK value checks take 0.014–0.040 s. Actual C1 VC5/native factory objects show two canonical operator-new/recon_cpp_new allocation edges and only the existing ContextBuffer::Initialize edge beyond them, with no constructor/placement helpers. This is compiler/object qualification, not App acceptance; CRT defaults and allocation behavior are unchanged. |
| Typed tmpnam boundary and shipped COFF constants | Prevent the unvisited RCDLL tmpnam import from escaping to host libc; expose LINK's shipped COMDAT selection and FAR_EXTERNAL constants. | `4232797ee`, `2da1bd9cc`. SDK-typed tmpnam always denies with status 125 for NULL or caller storage; no successful name, allocation or buffer semantics are supported. Four GCC/Clang default/VC5-profile builds and checks pass in 1.545 s wall time (checks 3.1–3.5 ms each); four i386 C/C++ SDK syntax/value checks take 14–39 ms. Both slices received independent source review. No original captures, runtime default expansion or App credit. |
| Typed WCHAR16 towlower and bounded VC5 ASCII support | Bind the reached MSVCRT import without host wchar32 conversion; authenticated provider analysis proves the default-C branch uses integer operations with no calls or global/error/FP writes. | `b3df988c9`, `1ccba02a2`. Only explicit510 ASCII with default LC_CTYPE/codepage/MB state succeeds; other profiles/states deny125. Four GCC/Clang suites pass in 1.541 s build/check wall time; ASCII/error/full108-byte-FP checks take 8.5/9.4 ms, generic denial checks 5.3/5.7 ms. RCDLL reuses unchanged original0122 and reaches matching frontier b209 (178.594 ms total, 35.988 ms execution; rebuild 0.657 s). Original per-call locale/errors/FP remain unmeasured; static proof and native observations are distinct. No API-only original capture or App credit. |
| Direct scalar delete, NULL no-op and SDK machine constants | Bind genuine scalar delete directly to recon_cpp_delete; the authenticated provider delegates to free, whose NULL path returns without heap/state effects. Add shipped R4000/ALPHA/POWERPC constants. | `d6c43a2b5`, `0a6dc478c`. Four GCC/Clang default/510 test groups pass in 5.961 s wall time (binding/NULL ownership, payload, error and full108-byte-FP checks 27.6–29.8 ms); twelve cached delete records and six non-NULL denials remain exact. Both ordinary Grow pairs reach Flush: C1 original/native 628.741/10.425 ms, accepted `34c95344f`; C1XX 1.499 s/13.146 ms, accepted `dda4735f5`. Four i386 SDK C/C++ checks take 14.5–40.6 ms. Independent reviews passed. Non-NULL ownership checks remain unchanged; no wrapper vertex, array-delete/exception expansion, API-only original campaign or shared App credit. |
| Explicit native self-command routing | Support a declared self-child invocation through the runner while retaining a fresh executable process and existing trace state. | `100852b60`. Eleven focused routing checks passed with GCC/Clang (0.0197/0.0241 s); eight existing process tests and both RSP2 suites passed. Runner support was subsequently published in `00331e531`; actual Both actual LINK self-child cases pass (DLL 121.461 ms, debug DLL 141.942 ms), and the final 91-case LINK gate passes in `8bd298b5d`. |
| Scoped expected RET cleanup contracts | Retain C2's existing calling-convention checks without imposing one global cleanup rule on every function. | `4ff437ad2`. 65 tests, including a negative test where both sides have the same wrong cleanup; C2's 15 contracts and 1,212 paired graphs pass. |
| Strided callback tables and exact import-site assertions | Express VC5.10 CVPACK's existing callback layout, explicit null rows and resolver-site inventory in the common audit engine. | `8b6ea0935`. 67 tests and ten denial checks; real CVPACK 113-graph audit passed in about 0.202 s. |
| More precise graph-audit failure diagnostics | Identify the owner, side, source and target of an unknown tail-call ABI instead of reporting only a generic failure. | `df221a941`. Exposed C1XX's specific pending debug-helper boundary; error reporting only, with no audit semantic change. |
| Mask immediates are not function pointers | Exclude TEST/AND/OR/XOR mask constants from function-reference discovery, while retaining actual address-bearing MOV/PUSH/CMP and arithmetic checks. | `a7f50d042`, `433be1cd3`. Both discovery paths have regression coverage; the full C1XX audit passed. |
| Canonical graph-registry adapters | Consume RCPP/CVPACK registries, resolve identity-only endpoint names and explicit PE import thunks, and reuse RC runtime-data checks without local frameworks. No names invent extents or body availability. | `dacb8c249`. Seven real profile checks, 70 tests, 12 denial controls and two unchanged legacy reports; canonical release matched the reviewed frozen bytes. |
| Object-qualified runtime anchors | Recover historical compiled CRT overrides despite duplicate source/library MAP names; allow explicitly classified pending CRT identity anchors without promoting their bodies. | `f3fbd2f1f`. Focused tests and one actual identifier/static audit recover RC's three historical matches; 193 graphs and 122 original/117 rebuilt support intervals pass in 327 ms. Five previously unaccepted declarations remain explicitly unavailable. |
| Native repository drive default | Restore the old native runner's `RECON_DRIVE_R` repository mapping before module load while respecting explicit case set/unset values. Wine prefix mappings are untouched. | `82451ad47`. Three real i386 default/set/unset controls pass, about 2.8 ms each; the canonical release is verified and RC integration passes in `8dc33696f`. |
| Same-entry runtime aliases | Use the next distinct COFF symbol boundary so genuine aliases share the actual body rather than producing zero-length records. | `205851271`. Eight focused tests and actual original/rebuilt catalogs pass: record counts and physical code unions unchanged, two zero-length aliases corrected per image. |
| Optional MAP-qualified runtime identities | Qualify byte/relocation matches against hash-bound rebuilt MAP names and addresses, retaining proven same-member aliases and rechecking external-reference closure. | `9a5ace9f8`. Ten tests and unchanged default-output comparisons pass. C1XX runtime records fall from 242 to 209, removing ten false overlap groups without changing the code union or reconstruction credit; about 403 ms versus 397 ms without qualification. |
| Normal-loader original DIRECT probes | Add a helper import offline to a scratch PE; let its local exception handler invoke original-compiler typed probe code and publish a read-only broker receipt. | `29265595b`, `934790b33`. Host/i386 writer tests, completion/frontier and four failure controls pass. Original controls take about 1.4–2.1 s; matching native controls about 5.5 ms. C1's first trial reached an unclassified shipped `_strncpy` boundary in 1.973 s; exact 37-byte COFF proof qualified it as CRT, without application credit. The actual typed pair then passed at RVA `3f0cd`: original 1.639 s, native 17.452 ms, identical first-entry order. Unused remote-call prototype cleanup remains separate. |
| Resume-capable native entry observer and exact dependency trees | A signal-safe one-shot observer inspects live entry state and resumes the genuine entry; optional read-only roots verify complete file membership as well as existing dependency hashes. | `a4954756a`. Focused observer controls pass (215 ms build, 3 ms run), membership tests and formatting pass. No descriptor ABI change; the optional capture field remains version 2. CL consumer integration passes in `6173606d3`. |
| Retained process-policy failure diagnostics | Preserve bounded private broker diagnostics before temporary-workspace cleanup, without mixing them into target stderr. | `fe1756fd8`. A rejected-input control passes in 7.299 ms; error details are bounded to 16 KiB. CL then identified a consumer image-input mismatch rather than a transport defect. |
| Plain native self-child execution | Public `native-child` command reenters the existing case with inherited arguments/environment/cwd/streams, fresh application state, shared RST1 tracing and readable child results. Existing process permission selects the route; no new wire protocol or original-binary changes. | `00331e531`. Eight actual canonical-release controls and existing smoke pass. Success takes 11.5–13.5 ms; a hung child is cleaned up at 505.8 ms. Child exit 7 reaches the parent; full DWORD is recorded separately, while the existing shim parent return remains eight bits. Handled traps and missing receipts still fail the fixture; INT3 owner and raised-signal diagnostics are distinct. 32 Rust tests, formatting and process/assets/output checks pass. Both actual LINK self-child cases pass (DLL 121.461 ms, debug DLL 141.942 ms), and the final 91-case LINK gate passes in `8bd298b5d`. |
| Clear completed-capture partial expectations | A completed original capture clears inherited frontier-only expectations instead of carrying them forward from its input capture. | Root one-line correction to original.rs; C1XX encountered and explicitly finalized the real completed case without recapturing. Formatting and all 32 Rust tests pass. |
| Qualified nonexact CRT identities | Permit CVPACK's explicitly reviewed original CRT variants to be classified as CRT while retaining identity-only graph projection, exact existing hashes/bounds and zero body availability/application credit. | `1e10445a3`. Two focused tests, formatting, Clippy and five CLI rejection controls pass. The actual 187-function audit takes 213 ms and its report is byte-identical to the old baseline. CVPACK mask verification restores only the three approved CRT bodies (1,253 bytes), retains all 20 App traps, and ten native cases/unit executors pass in 161.8 ms. Consumer correction is committed in `d7264dd14`; final audit passes in 4.708 s including original-compiler prerequisite rebuild and identification. |
| Canonical pending CVPACK drivers | Validate pending App identities, canonical extents, text-data and uniqueness while retaining stub status, no body availability or acceptance, and zero App credit. | `c2739c352`. Three adapter tests, focused rejection controls, formatting and Clippy pass. The frozen 208-function report is byte-identical (225 ms); an explicitly derived 207-plus-pending view passes (231 ms). No speedup claim. |
| Original-only CRT instruction facts | Export explicitly selected, hash-bound canonical CRT spans through the existing decoder without inventing a paired NT body, checked ABI, execution permission or App credit. | `307e3ed37`. Seven instruction-export tests, 15 existing paired controls, 14 focused support controls, formatting and Clippy pass. The actual WCC startup/CMain sample has two owners and 53 instruction rows (about 19 ms); the offline consumer verifies bytes, relocation and null checked cleanup. Default and paired CF reports remain byte-identical. No capture or speedup claim. |
| Preserve application errno across host work | Fresh executors establish initial errno before loading; post-constructor descriptor metadata preserves constructor changes. Setup/callback checkpoints restore their own values after host preparation, and actual return errno reaches exit callbacks. | `e0116ffcb`. Native GDB proved host realpath/readlink changed errno to 22 after constructors in LINK. Constructor 77, setup 88, callback/atexit 99, fork/nonfork units and real SELF controls pass; baseline runner fails. Canonical controls take 2.8–3.0 ms for IO/DIRECT/no-setup, 4.8 ms for mixed units and 11.4 ms for SELF. Existing smoke/eight SELF/32 Rust tests pass. No library, App, fixture expectation or errno-mask changes. |
| Separate application, CRT and harness builds | Default source discovery covers src/crt, explicitly selected C++ harness support uses C++ compiler/flags/linker, and quote-only local include paths preserve system headers. | `f71bc7e1c`. Focused parallel/incremental native/original Make tests pass, including C++ support and a local link.h shadowing regression. Existing explicit source lists remain supported. |
| Optional registry support-extent guards | Structurally walk reviewed CRT/import intervals independently of accepted application graphs, reporting unavailable rebuilt bounds explicitly. | `493a69af8`. 71 tests and focused denial controls passed; canonical release matched frozen validation. RC's initial 114 rebuilt intervals became 117 after the qualified runtime recovery above; all 122 original guards remain. Default reports and graph/RET scope are unchanged. |

The runtime changes separate classification, available native body and reviewed
graph evidence. They do not add CRT reconstruction or silently waive application
graph checks. Explicitly qualified historical CRT vertices remain checked.

## Changes to lib/recon

The shared library maintainer made bounded changes for the new loaded
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
for this shared-library work. Native-child/broker wiring was subsequently
published in `00331e531`; an actual LINK self-child integration check is still
pending. Passing the library tests alone does not establish that integration. The
maintainer currently has no additional shim edits or tests in flight.

The obsolete glibc-tunables consumption constructor and its dedicated tests,
Make target, documentation and validation receipt were removed in `6d221d71a`
after CVPACK dropped its sole use. Remaining entry/FP/status code is byte-identical;
the focused precision and initialization checks pass in 2.598 s. This removes
128 lines across six files and introduces no replacement allocator feature.

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
The proposed CVPACK startup-reexec/allocator-profile feature was cancelled. C1XX's
routine nine cached cases and 16 unit groups also passed with this exact runner;
they were not rerun solely for the publication.

The final CVPACK batch subsequently exposed two output-padding differences.
Static analysis identified only unwritten filename-alignment bytes and a
checksum derived from them. A bounded diagnostic restored the old runner's
glibc startup tunables and reproduced both raw hashes, proving that migration
had omitted an old launch prerequisite. The user rejected this kind of test:
allocator tuning to reproduce unwritten padding is not a useful correctness
contract. No startup-profile/reexec feature was implemented. The user then
explicitly directed the reconstruction to zero this padding and document the
original defect in source, then required masking proven uninitialized bytes in
comparisons. The bounded source fix is verified on both platforms in
`b1475f9bf`. Only the two established filename tails are masked; the derived
hello checksum is masked after independent correctness validation. The callback
also requires the reconstruction's padding to be zero. Raw references remain
preserved. No general output parser, duplicate artifact assets or allocator
profile was added.

CVPACK's prior README incorrectly made historical conditional CRT guards a
prerequisite for resuming reconstruction. They whitelisted specific binary-mode
`__sopen` invocations and one no-op `__freebuf` path because the embedded CRT
variants differ from the shipped library; `_setvbuf` had no authored runtime
guard. Those permissions do not cover the queued strip case. The correction
classifies the three known, hash-bound variants as CRT identity boundaries,
leaving their original bodies executable under the existing masker. Their
nonexact differences and old guard facts remain documented; no CRT source,
whole-library equivalence claim, new runtime guard or reconstruction credit is
introduced. The shared adapter keeps these identities outside body graphs.

CVPACK's correction is committed in `d7264dd14`; the user subsequently
authorized autonomous reconstruction, which has resumed.
All three variants are CRT definition 3 / implementation 0, while historical
graph anchors remain accepted false / unmask false. The production mask check
changes only their 1,253 bytes and leaves all 20 unfinished application traps.
The final audit passes 187 graphs, 206 data objects and 207 identities. No
application bodies, capture hashes or acceptance counts changed in that correction.
The queued strip case subsequently qualified in `2cac9d068`: fresh masked
original capture took 1.228 s, with 34 App first entries; native replay took
41.414 ms and its unit 10.351 ms. Eleven cases now run, with no new body credit.

The reached C1XX readonly diagnostic-stream refill difference is corrected in
`bbd1bf5f6`: after the first successful 512-byte read into an established owned
4096-byte allocation, the default shim restores logical capacity 4096. Explicit
VC5/Watcom profiles are unchanged. Focused capacity/ownership/error controls pass;
C1XX adoption `f3ea9a154` matches the saved original and actual refill state
(32.182 ms), with ten cached cases and 21 unit groups passing in 0.281 s. Shared
rebuild took 0.746 s and audit 2.144 s. Earlier steps 44–45 matched outputs/order
but lacked this state qualification; that limitation is recorded without
relabeling raw evidence or rerunning originals.

## Recent shared capture work

The following corrections are published and verified:

| Work | Consumers and purpose |
| --- | --- |
| Partial output normalization | `608025b10` selects partial output rules before collecting frontier artifacts. An empty partial object no longer inherits the final COFF timestamp mask; final bounds checks remain strict. Formatting and 34 i686 Rust tests pass (0.02 s). VC5.10 C2's next fresh pair succeeds: original 1.859 s, native 11.402 ms. |
| Original-capture environment separation | `54bb2913f` separates case target-environment overlays from host Wine transport and resolves the Wine executable using host PATH. A real original control verifies empty target PATH, removed target Wine variables, retained defaults and target overrides: capture 1,735.471 ms, native 5.522 ms. Formatting, 33 Rust tests, C environment-merge controls and native smoke pass. CVPACK's queued strip capture subsequently succeeds. No extra Wine process or private prefix. |
| VC5.10 capture prerequisites | `b36c3c674` preserves parsed PE loader metadata without App overlap, stages unchanged static hosts and declared toolchain-local imports, verifies an explicitly selected native MSVCRT file, and records frontier registers/FP plus 20 stack bytes. CVPACK's new baseline drops accidental empty PATH inherited from native harness sanitization; historical evidence remains unchanged. The actual pre-entry cause was helper base `0x10000` relocating to `0x110000`, triggering Wine's `start.exe` fallback, whose builtin dependencies failed under the native override. Linking the tooling helper at `0x400000` and passing its absolute Unix path fixes this without changing target PE bytes or the shared prefix. Broker-only qualification passes; the genuine CVPACK control reaches `f5f0`/112 entries in 1,319.479 ms with the exact native provider confirmed. Canonical release/helper builds take 13.486/2.902 s; formatting, 38 Rust tests (0.11 s), native smoke, eight SELF controls, errno controls and environment stand-in pass. Full identities and preserved diagnostic packets: `/root/.ai-tmp/msvc2/vc510-shared-control/READY.json`. |
| Static-DLL loader breakpoint ordering | `e0a52f698` identifies the genuine first-chance, main-thread ntdll loader breakpoint by bounded export/site bytes and consumes it once. Static DLL attach no longer prematurely marks that breakpoint consumed; other external traps reject. Frozen RCDLL original reaches `12974` with 94 entries in 1,459.659 ms; native matches in 177.806 ms. Existing EXE smoke passes in about 1.161 s. |
| Bound-import header space for DIRECT | `2cea3119e` relocates a validated bound-import block into appended tooling data, freeing one section-header slot without moving original sections, RVAs, code, resources or relocations. Offline preservation/malformed-input controls pass. Frozen CL typed DIRECT reaches `3b8e` with 56 entries in 562.142 ms and matches native. |
| Opt-in raw artifact export | `264347933` adds `--artifacts-dir NEWDIR`, preserving exact files and a hash/path manifest before normalization and cleanup. Default capture and fixture schema stay unchanged; safe-name, collision and symlink containment controls pass. Frontier stack diagnostics now retain 64 bytes. Real C1XX Ledger export retains seven outputs in 1,108.292 ms; CVPACK retains its final 63,960-byte output in 1,569.391 ms, locating one proven uninitialized padding byte. Neither diagnostic replaces an oracle or relaxes comparison by itself. |
| Folded CF diagnostic transport | `c52c33771` exposes explicit WCC entry-stop through the shared supervisor with no App oracle or CF body execution. Corrected thread denial, existing four-return positive and wrong-PC denial take 982.778/1,470.722/875.519 ms. Public entry-stop takes 2,804.297 ms and validates actual relocation, launch inputs and environment. Ordinary catalog/progress/oracle support remains in progress; the exact first real WCC frontier path is recorded below. |
| Code-free build aliases | `097ce4cac` adds optional `RECON_ALIAS_MAP`, emitting zero-section COFF weak aliases and native linker aliases from one validated binary map. One canonical body/address remains in the graph. Offline emitter, native real-vtable equality and missing-target controls pass; VC5 compile/link/MAP controls take 1,428.111/683.111 ms. C1XX production adoption `d74c0cd10` passes 104 body audits, actual PE/ELF slot equality, the factory unit and its fresh frontier pair; cached checks/units take 0.024 s. |
| Explicit fresh-pair report comparison | `0bf036e0d`/`30f98e94c`: `--original-report` compares native trace and terminal/frontier directly to the selected original receipt; ordinary cached replay retains descriptor expectations. New receipts bind exact capture SHA and binary/case/execution identity, while explicitly selected legacy reports record unavailable source binding. Explicit fresh progress tolerates the former descriptor frontier becoming implemented, while the selected frontier and cached replay remain strict. The exact saved LINK discrepancy previously printed PASS using provisional descriptor expectations: original `31f30`, native `616d0`. The new gate rejects it in 27.489 ms; corrected native matches all 128 entries and `31f30` against the untouched original in 21.563 ms. No original rerun. Formatting, 47 Rust tests (0.26 s), native binding/terminal/partial-oracle controls and existing smoke pass. |
| Ordinary CF progress and controller consolidation | The shared frontend writes a bound partial I/O oracle and report at the genuine WCC MemAlloc `47c77` / EAX 8 frontier through five support owners totaling 397 B; no pending App body or gateway executes. Adaptive shared-map waits preserve protocol/deadline checks: authored 40 / 30 step controls pass with guarded execution of 37.179 / 52.781 ms. The genuine WCC run has 10,848 guarded steps and 73,783 RPCs in 9.042 s guarded / 11.082 s overall; frontend CPU 9.774 s and waited-child CPU 5.895 s exclude the shared wineserver. All 49 Rust tests, public projection / five-owner graph parity, 19 identity / 19 IPC / 17 native-child and 51 gate controls pass. Identity-only CF projection and explicit CRT witnesses preserve registry roles and extents. Relevant tests and consumers move to recon-tool; the separate capture folder is retired with exact prior source preserved in Git checkpoints. |

The earlier shared worker was repeatedly stopped by the platform's
possible-cybersecurity-risk flag while preparing self-child support. At the
user's request a fresh worker implemented the simpler public-command design
from published sources, without consulting the earlier scratch implementation.
No sealed context, authorization journal or hidden command was added. A focused
fixture initially assumed implicit repository R: mapping; the cause was proven
by explicit mapping and corrected to guaranteed supervisor T: mapping. The
failed log remains preserved. This was a fixture prerequisite error, not a
claimed concurrency defect.

CL initially carried forward the old suite's whole 894-file SDK inventory in
all 57 cases. The user rejected that overinclusive check; CL has removed it,
retaining validation of the specific child executable and actual captured
workspace inputs. No per-child file-open readset is claimed. The shared
optional tree feature remains available, but is not a CL requirement.
The entry observer serves three existing cases that start through `main`,
intervene at a genuine internal function entry and then assert resulting I/O.
For example, one selects OMF response generation at `link_pass`. Their current
IO classification describes output assertions; these are not untouched CLI
invocations.

CL final verification passes after removing the whole-SDK check: all 57 native
cases take 0.631 s wall at `-j4`, with all 90 typed child requests validated.
Six targeted rejection controls pass; the unit command visits all 57 case
libraries in 0.468 s. Original compilation takes 3.162 s and the strict
110-function graph/RET/ownership/extent audit 0.291 s. Clean native compilation
takes 2.370 s. Migration and obsolete-framework cleanup
are committed in `6173606d3`; CL is idle with a clean scope. Its 104 application
and six historical CRT bodies are unchanged. All 67 initial files, 118 raw final
masters, 936 raw payload records and trace seeds were checked against the old
captures without recapturing originals. Each case now lists at most five specific
external dependencies (child executable images and CL.ERR/CL32.MSG).

LINK's complete migration and layout cleanup are committed in `8bd298b5d`.
All 91 cases and seven narrow tests pass in 1.697 s wall at `-j4`; its host-only
840-function graph/RET/reference/ownership/extent/runtime audit takes 1.464 s.
Native incremental build takes 3.656 s; the earlier combined layout build/audit
checkpoint took 8.976 s. All 803 App functions, 16 historical CRT overrides,
raw masters and held demangler WIP are preserved. Fifteen buffered phases needed
their frozen LINK32_DB=27 input restored. Three IO cases needed only the old
phase-declared clock exclusions restored; four direct reports needed the shared
errno fix. No new masks, expected values, original captures or App promotion
were used to resolve those differences. LINK is paused after its commit.

Root additionally ran native `make -j4 check unit` for the then-idle five VC1.0
and all nine VC5.10 binaries against the errno-fixed runner. All 14 commands
pass, with command wall times 0.162–3.052 s including any incremental build work;
two binary commands ran concurrently. Per-binary results and logs are preserved
at `/root/.ai-tmp/msvc2/errno-adoption-regression/results.json`. C1 and C1XX also
adopted the fixed runner in their ordinary cached gates without recapturing.

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

The old `decomp/vc1.0/fixtures/` directory is retired after all eight migrations
passed. Its 674 tracked files remain in Git history. A verified full snapshot,
including ignored/untracked contents, is preserved at
`/root/.ai-tmp/msvc2/retired-vc1-fixtures-20261025T135743Z/`: 681 entries and
36,196,105 logical regular-file bytes, checked before deletion. There are no
remaining live source dependencies on the tree. Seven obsolete graph parity/
benchmark helpers and their dead README commands were retired in `b4ace6f40`;
Rust code, useful tool test fixtures, profiles and historical timing receipts
remain. RCPP's separately missed 27 historical console-interrupt evidence files
were removed in `c89282a38`, with their Git/scratch copies preserved. No new
capture or reconstruction was required for this cleanup.

The stricter canonical registry also exposed a VC5.10 C2 source identity issue:
17 distinct callback slots used a separate trapping placeholder for an original
entry already represented by a real CRT function. A shared per-owner alias
waiver was rejected. The bounded canonical-pointer repair and consumer
inventory are verified and committed in `dccbd1906`; original compilation,
strict 111-function graph/reference checks and both cached cases pass. It adds
no reconstruction credit.

## VC5.10 readiness reread

All nine actual workers reread the current guidance and their own documentation;
that readiness review was read-only. The user subsequently released the seven
unfinished binaries to implement the minimal capture prerequisites and finish
their current cases, then pause and report before moving beyond them. RC and
CVTRES remain complete for identified application code and stay idle. The counts
below include verified resumed work and exclude current unaccepted candidates.

| Binary | Accepted App functions / bytes | Whole-text coverage | Current frontier / plan |
| --- | --- | ---: | --- |
| C1 | 116 / 17,712 | 5.50% | Minimal C: translation-unit initialization after type/name interning; `dc5cddcd4` |
| C1XX | 129 / 18,682 | 2.49% | Minimal C++: global/scope setup after own type interner; `97bd8ce54` |
| C2 | 157 / 37,497 | 6.54% | Numeric case: register_types_match after address-register construction; `017c823a3` |
| CL | 109 / 18,279 | 95.89% | Bounded live Job-chain DIRECT complete, committed `677a11cab` and paused; 14 public cases unchanged |
| CVPACK | 124 / 40,643 | 64.86% | Debug DLL complete, committed `d967c21fc`; paused before another case |
| LINK | 146 / 48,010 | 11.57% | Hello EXE: module-symbol processor after relocation completion; `4d27b6fcd` |
| RCDLL | 123 / 33,633 | 45.08% | Rich corpus: resource-name duplication after type lookup; `e06b3898e` |
| RC | 2 / 277 | 19.26% | Complete: all identified App code |
| CVTRES | 22 / 7,445 | 76.80% | Complete: all identified App code |

CL has a separately supported 98.57% identified-App percentage; other unfinished
non-CRT denominators remain unclassified. Whole-text ratios include runtime,
data and padding.

The current C2 descriptor-byte check caught a signed-short field-code error
that matching trace/output values had hidden. Its corrected source, focused
signed-value check and fresh native run reuse the unchanged genuine original;
the failed assertion remains recorded. RCDLL similarly retained a cleanup
timeout caused by an omitted typed header, corrected the actual AX return-width
use, and now treats implicit function declarations as native compile errors.
Neither correction required an observer framework or another original capture.
LINK's next candidate exposed an unvisited wrong-field read and, separately,
the fact that runner `--capture` selected inputs/IO while trace/frontier checks
still used descriptor expectations. Its independent actual-report comparison
caught the divergence before promotion. The source correction reuses the
unchanged original. Shared releases `0bf036e0d` and `30f98e94c` now provide an
explicit `--original-report` comparison and permit stale descriptor expectations
only on that selected-report path. The saved mismatch rejects, the corrected
native matches, and LINK's acceptance has resumed. Other active workers use the
same explicit gate; no historical replay campaign is required.

RCDLL's attempted result-storage merge in `4f7a7b5e1` was later disproved:
the resource reset/parser result is RVA `25d3c`, while command initialization and
the main return use distinct RVA `1853c`. The earlier review had verified the
resource operand but relied on a source name for the supposed shared owner.
That was insufficient. Correction `f534ae93b` restores distinct storage and
binds both actual consumer chains, a bounded 17-field reset/consumer map and
noncoincident tests. A separate two-consumer check maps parser callback context
and main HeapDestroy to the same real heap owner (`1d87c43c`). Both corrections
have zero coverage credit; mistaken history and equal-zero limitations remain
explicit, with unchanged originals reused for fresh native checks.

The concrete minimal capture work is:

- C1/C1XX have implemented and original-compiled small typed three-argument
  compiler-export hosts in each binary's `harness/`, using shared `--host`.
  Both hosts now have successful genuine original/native frontier pairs.
  Old HOST argv/environment vectors and forced LastError values were observer
  choices, not required App inputs.
- C1's next genuine inline object construction required small compiler
  portability support: `477bd3d95` supplies qualified inline/placement-new
  declarations, and `aaa964cd1` denies unsupported native pure-virtual calls.
  Original/native compiler experiments preserve the 8/20/88-byte layouts and
  real allocation/initialization edges without extra constructor vertices.
  This is compiler support, not application acceptance or CRT reconstruction.
  The shared tree is published and stable at `446d1949e`.
- Both frontends next encounter vtables whose different source methods share
  one original physical function. The approved accommodation uses code-free
  weak COFF aliases and native linker aliases from a binary-specific map,
  preserving one canonical body/registry entry and actual pointer equality.
  The shared build helper is published as `097ce4cac`; both workers are released
  for ordinary production links, vtable/ABI checks, audits and fresh pairs.
  Scratch VC5/native tests and Make wiring pass without wrappers, raw vtables,
  duplicate body credit or a graph waiver. No additional helper Wine controls
  or preliminary target capture is required.
  Both production integrations now pass: C1's two 24-slot PE vtables establish
  all 17 alias equalities, and C1XX's PE/ELF vtables share its pending Reset
  entry. Normal strict audits and real-object native units pass. C1XX's first
  attempt exposed a genuinely masked six-byte operator-new import thunk;
  the authenticated import endpoint was added with zero App credit before
  the required corrected-mask original/native pair.
- RCDLL reuses shipped RC.EXE; a new dynamic-loading host would unnecessarily
  change its static attachment lifecycle. Its IAT/debug/import/export data in
  executable sections must survive progress masking as verified data ranges.
  Parsed loader-data preservation and private host staging are published in
  `b36c3c674`. A subsequent static-attachment ordering issue is diagnosed:
  the broker consumed the loader-break allowance before Wine's real initial
  process breakpoint. The narrow fix is published as `e0a52f698`: only the
  initial main-thread ntdll process-breakpoint signature is consumed once;
  unrelated traps remain errors. The genuine qualified original (1.460 s)
  matches the native case (177.806 ms) and is reused without a publication-only
  recapture.
- Exact MSVCRT identity is required only when reusing provider-specific table or
  instruction qualifications. C2 withdrew a universal hash blocker. Intended
  native-provider selection plus once-per-capture loaded identity is enough
  where such proofs are used; no per-function scans or CRT reconstruction.
- CL's 91-byte console_rows and separate 62-byte control handler remain
  explicitly pending. The one ordinary two-source warm-IDB MRE retry completed
  successfully but did not reach the 41-byte helper; it gains no App credit.
  The approved fallback is one NULL/tail/head typed DIRECT probe on the genuine
  live Job chain at prepare_jobs. Its first attempt was rejected before Wine
  launch: bound-import metadata occupied the spare section-header slot. Shared
  fix `2cea3119e` relocates only that validated loader block into the helper
  section, preserving target code, sections and RVAs. The subsequent original
  and native DIRECT completion passes; no local PE loader, console machinery
  or MRE emulation was added.

CVPACK's empty target PATH was copied from old native harness sanitization during
migration, not captured as an application requirement. Its genuinely new baseline
removes that override and other unrelated harness controls, preserving the old
capture/history. Removing it did not by itself fix the observed pre-entry failure;
the separately diagnosed Wine launcher/provider correction is now published.

The target-environment separation fix is published as `54bb2913f`. Shared
capture release `b36c3c674` adds loader-data/host staging, explicit target-only
native-MSVCRT selection and actual frontier registers/FP plus 20 stack bytes.
Its genuine CVPACK qualification reaches the next frontier in 1.319 s and was
reused for the matching native acceptance, rather than repeated. Workers
are active only within the released current-case scope; no replacement was
necessary. New ordinary-IO frontier state needs and capture transport failures
are routed to shared tooling, not local observer frameworks.

The independent C2 partial-output collection fix is published as `608025b10`:
frontier captures choose partial output rules before normalization, while final
COFF bounds checks remain strict. C2 verified it with fresh original/native pairs
and strict graph/ABI checks, then committed six application advances through
`176abee09` (register insertion, node construction, clearing, input-state reset,
function reader and scope allocation). A normal/PCH selector error exposed
beyond the previous child frontier was corrected without extra credit; the
unchanged original reference was reused with fresh native evidence. It
continues the original numeric case; no additional case has been started.

C2 has reconstructed the complete 7,202-byte hot/cold IL dispatcher and its
exact owned switch data, reaching the first pending token-reader child in a
fresh 1.337 s original / 11.668 ms native pair. All 133 graphs and 16 units pass.
This does not claim dynamic coverage of its 82 dispatch targets: unknown opcode
property rows remain guarded and unmodeled, with no raw 160-WORD table copy.
Subsequent token input is committed as `c156f83f8`, reaching the next decoder.

CL's bounded completion adds the 41-byte deferred-job helper: 109 application
functions / 18,279 bytes. The genuine original DIRECT run passes in 1.328 s;
native passes in 7.661 ms (4.416 ms setup, 2.511 ms execution, 0.734 ms checks).
The original-emitted result and live ownership/link/ABI assertions cover NULL,
tail and head inputs. All 15 discovered cases (14 public IO plus one internal
DIRECT) and their tests pass in 0.602 s including case rebuilds; original build,
full 124-vertex audit and qualified 112-owner audit pass. This establishes
internal behavior, not ordinary public MRE reach. The remaining 266 App bytes
are the deferred console/control paths and 113-byte pass toggle.

CVPACK completed the released debug-DLL case and paused at `d967c21fc`, adding
11 application functions / 3,624 bytes. The final original exits successfully
in 1.045 s; native execution returns the same status and exact complete DLL
SHA-256 `935d7828c7003e4f951e79cb4a709c747eac8006405ac996299dea4e64dd0d40`
in 14.615 ms (5.339 ms setup, 7.852 ms execution, 1.424 ms checks). Both cached
cases, 13 narrow tests and the strict 124-function graph / 123-owner RET audit
pass in 0.287 s; final incremental native/original builds take 0.201/0.043 s.
The existing qualified dependency contract is unchanged. A proposed larger
multi-TU workload has not been started.

## Watcom compatibility report

The initial compatibility assessment was read-only. Shared integration has since
reached the transport milestone below; application reconstruction remains held.
Workers may statically classify the finite startup paths while shared progress
and oracle support is implemented.

The user subsequently approved integrating this backend into shared
`recon-tool original capture`. The shared recon-tool owner implements the
integration; the Watcom coordinator supplies controller contracts and review.
After verified shared READY and root release, each worker may replace the old
minimal source/cases with a fresh standard scaffold, complete one actual help
case, then pause. No application runs are released before that checkpoint.

| Binary | Accepted application reconstruction | Current paused state |
| --- | --- | --- |
| WCC386 | 0 functions | CF main stub; cached `?` output is not a captured application entry order. |
| WPP386 | 0 functions | CF main stub; cached `?` output, no accepted application prefix. |
| WASM | 0 functions | CF main stub; guarded patched `/?` output, no accepted application prefix. |
| WLIB | 2 functions / 143 bytes | Accepted main/hook prefix; `InitMsg` is unaccepted WIP and current fixtures explicitly HOLD. |
| WLINK | 1 function / 232 bytes | Native main-to-`Spawn` prefix works; `Spawn` remains pending and excluded from production. |

**Already shipped.** The native i386 DSO runner, flat captures, typed native
entry hooks, static-symbol tracing, full DWORD receipts and parallel check/unit
targets apply to all five. Public native SELF children, captured dependency
responses/assets, module ownership and errno preservation are available; none
requires a Watcom replacement. Original target-environment separation
(`54bb2913f`) and fixed R:/S:/T: transport are also available. Old captures retain
their actual environment/path provenance; this fix does not rewrite them.

Shared original-tool launching already supports Watcom and pins the patched
WLINK hash. Original Make commands can be overridden for WCC386/WLINK; Windows
header case/backslash dependency resolution is fixed. Graph-audit already reads
CF images and real Watcom MAPs (`--map-format watcom10`). Registry ABI 2 preserves
`original_symbol` separately from the native symbol: e.g. `main_` versus `main`.
The opt-in Watcom FP, first-main heap and normal-only 52-byte setjmp support is
published. No concrete additional shim or PIC implementation is currently needed.

**1. Native migration prerequisites.** All five can use the current native
runner; it does not execute their original PE/CF format. Binary workers can move
application code to `src/`, glue to `harness/`, and use shared Make rules with
Watcom compile/link overrides. The user permits replacing the old minimal target
sources/cases; Git history suffices. Pending `InitMsg`/`Spawn` receives no credit
through that reset. CF output-only caches
must not be presented as complete trace or frontier oracles; missing entry
evidence stays explicit.

Two minimal audit accommodations remain; the graph maintainer confirmed the
existing CF/MAP reader and catalog interfaces:

- **CF registry projection — recon-tool owner:** `src/projection.rs::registry`
  currently forbids `policy.target` and constructs a PE-shaped target. Add a
  bounded way to retain the exact existing CF `target.path`/`target.image`
  descriptor, loader/container/relocation hashes and payload offsets at base 0.
  Use graph-audit's existing CF reader; no synthetic PE or second CFG walker.
  Entry-only CF names belong in the existing identity registry; do not invent
  one-byte or whole-image code extents to make them owned function records.
- **Reviewed runtime catalog reuse — recon-tool owner:**
  `src/runtime.rs` still requires COFF/ar input and a `.text` section. Watcom's
  CLIB3R objects/libraries are OMF and its Win32 code uses `BEGTEXT`. Preserve and
  validate the current explicit runtime identity/extent evidence through shared
  tooling instead of rebuilding a general OMF matcher for this migration.
  Known names alone cannot supply bounds, body availability or CRT exemptions.
  Graph-audit already accepts flat `--runtime`/`--rebuilt-runtime` records
  (`src/index.rs`); those inputs do not authenticate arbitrary JSON hashes.
  The shared ingress must check the reviewed catalog, image/body/relocation
  evidence and current rebuilt PE/MAP provenance before projecting records.

DSO constructor ordering, WLIB's empty target-allocation namespace and actual
entry FP state need migration validation, rather than new shim semantics. Case
setup uses host allocation paths so it cannot falsely mark target activity.
Native GCC ABI is explicitly authorized; original Watcom builds retain true
watcall/signature/export checks. No per-function veneers or OMF-to-ELF transport
are needed. The exact WLINK-main noreturn residual and Watcom register ABI checks
remain target-specific; RET cleanup checks do not prove the whole ABI.
CF import and callback-table checks remain unsupported; their availability must
not be inferred from the existing CF reader or native typed-hook support.

**2. Original capture integration.** Shared `original capture` now has a finite
WCC CF entry-stop mode in `c52c33771`. It reuses the existing controller behind
the shared frontend, with exact payload/loader identity, base-zero coordinates,
full relocation verification and separate target environment inputs. It does
not yet provide ordinary CF progress/oracle capture. Mixed CF code/data cannot
be bulk-masked. WLIB can use the ordinary offline PE masker: its DGROUP tables,
resource overlay and file length survive. The missing prerequisite is a finite,
validated transitive startup CRT/import closure, including actual code and any
demonstrated owned text data. Existing direct-entry hashes omit some called
helpers and executable import thunks. Reaching a callee during startup alone
does not classify it as CRT; application callbacks stay masked. The old
bootstrap-then-mask/two-mode transport is not itself a requirement.
WLIB/WLINK/WASM patched originals also require their existing
GS-patch provenance and all-invocation selector/buffer-owner guards; a hash alone
does not establish the patch's semantic conditions. Retain verified CRT bootstrap
boundaries. Preserve actual stdin semantics: shared original capture supports
NUL/file, but WLIB's saved pipe input is not yet supported there. Cached native
replay does not require implementing or rerunning this capture path first.
Fresh cases may explicitly capture staged argv0/self-open paths rather than
permanently retaining historical path quirks. Missing CF FP evidence stays
explicitly absent or unsupported; full DIRECT state is not a blanket initial
I/O migration prerequisite.

The published milestone preserves the original eight authored results under
their original bindings. The corrected thread denial now passes in 982.778 ms;
the folded private backend also passes four-return prefix progress in
1,470.722 ms and wrong-PC denial in 875.519 ms. These are backend controls,
not ordinary public CLI capture. The public frontend's WCC entry stop takes
2,804.297 ms, compares all 532,636 initialized bytes and 11,801 fixups, verifies
four requested environment keys and confirms owned teardown. It executes six
loader instructions, observes the actual CF-entry INT3, and executes zero CF body
instructions. App trace is empty, FP is explicitly absent, and no oracle file is
written. An earlier OsStr serialization failure occurred before any Wine/helper
launch and remains preserved. Exact pre-port controller WIP is recoverable in
`4d957bb5b`/`409be007c`; old folder compatibility shims are temporary. Final
integration removes `tooling/watcom-capture/`, its tests and all consumers.

Finite static evidence now identifies WCC/WPP's three and WASM's four XI
callbacks as CRT using shipped-library bodies/fixups. Callback order is derived
statically, not observed. WASM's `main_init`, `ObjRecInit` and `GenMSOmfInit`
remain App code inside main. Original-only CRT structural export is now published
in `307e3ed37`: it supplies bound original instruction facts with no invented NT
counterpart, checked ABI or execution permission. Arbitrary gateway/far-service
transfers, callbacks/SEH/APCs and new threads stay unproved or fail closed. This
milestone does not release the five application workers or establish ordinary
capture readiness.

**Shortest path to WCC's first real help frontier.** Static inspection found that
CRT FILE initialization calls the compiler's private allocator at CF offset
`47c77`, before main. Its source-correlated memory manager is application code,
not shipped CLIB `nmalloc`; the first real App call passes size 8 in EAX and
returns to `2f4d8`. No gateway service precedes that entry. The following finite
work replaces a broad startup-CRT campaign:

| Blocker | Owner | Minimum action |
| --- | --- | --- |
| Ordinary public CF capture | Shared recon-tool owner | Finish catalog admission, actual App entry traces, frontier/raw/oracle finalization; qualify the existing authored arena through the public frontend. |
| Five original support owners | WCC worker supplies witnesses; shared owner validates/consumes them | Bind `_cstart_` `2fe00..2feb4`, `__InitRtns` `48526..48571`, `_init_stk` `482c6..482ce`, `__InitFiles_` `4a1e5..4a25d`, and allocation wrapper `2f4cd..2f4db`. Preserve actual complete extents separately from execution subsets; stop at App `47c77`. |
| Reached startup instructions and data | Shared recon-tool owner | Qualify operand-size-16 MOV/selector stores, LODSB, bounded REP BSS clearing and flat-segment PUSH/POP. Validate actual launch block, environment, stack, exact BSS `7c448..80774`, startup globals, selector, XI and accessed FILE spans; do not grant the entire allocation writable. |
| Equivalent native startup | WCC worker after release | Add a target-specific bootstrap under `harness/` that performs the proved CRT-to-App allocation callback with its real arguments, order and state effects. Host CRT will not call this allocator automatically; trace-only calls are insufficient. |
| Later progress in the same help case | WCC worker and shared owner when reached | Reconstruct the App allocator normally. Its later AH=48h boundary dispatches to actual loader `410e27` and `LocalAlloc(0, EBX)`, initially requesting 65,536 bytes; bind returned ownership and actual flags/effects. Keep conditional fatal/OOM, unknown services and asynchronous execution closed. |

The wrapper's LEAVE and the memory-sourced startup stack reservation occur after
the first allocator call returns; they are later progression requirements, not
prerequisites for the first frontier. The exact five-owner witness, including
bytes, CF fixups, return contracts and first-entry effects, is
`~/.ai-tmp/msvc2/watcom-shared-integration/wcc-startup/first-frontier.json`
(SHA-256 `d29b80632db9a5f9b30fc48dcf703e98ac205e0d613288cf514cb316aedcfa0e`).
The actual loader service mapping is separately documented in
`wcc-gateway-minimum/summary.md` under the same scratch root. These static packets
and planned adaptations authorize no application execution; all five workers
remain paused until ordinary transport is qualified and released.

**3. Only when a DIRECT case is requested.** The typed original helper is already
implemented; do not add a generic watcall remote-call engine. Its current
`build-direct` hardcodes CL/LINK switches, libraries, entry/export spelling and
MSVC breakpoint syntax. The recon-tool owner would add a small Watcom helper-build
profile, using compiler-generated typed calls and explicit exports. Its probe
currently exposes only saved ESP/ECX/EDX; a reached Watcom entry using EAX/EBX
also needs those actual saved formals exposed in a versioned context. This does
not require native GCC to imitate watcall. The current typed backend's PE marker
identity cannot directly address a relocated CF payload, so CF DIRECT support
waits for a concrete requested marker and the existing controller adapter.

Existing FILE26, signal and nonlocal-jump denials remain explicit. No general
allocator, DLL loader, ABI engine or new shim family is proposed. This report
records support needs, the bounded transport milestone and conditional release
stages; it claims no newly accepted application reconstruction.
