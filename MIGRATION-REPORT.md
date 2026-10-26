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
  cleanup. C1XX, C2 and CVPACK are reconstructing; C1 has completed all identified
  application stubs and retains two unattributed routines. Finished binaries
  are included.
- **VC5.10:** all nine binaries are in scope, including finished RC and CVTRES.
  All nine have committed their migrations. Their 38 migration-checkpoint
  invocation cases, original builds and applicable graph/ABI checks passed;
  seven workers have resumed reconstruction, while finished RC and CVTRES remain idle.
- **Watcom:** shared capture integration is published and fresh scaffolds are
  being migrated and verified. The five initial help cases are in progress,
  with some awaiting shared support; none is complete yet. Details are below.

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
| Stable original header dependencies | Keep shared ABI/trace/inline annotations in recon.h and move native shim declarations/remaps to the native-only include root. Original search paths retain genuine common/project/SDK dependencies without a preprocessor, cache or exclusion rule. | `0aec8d902`. Native-only header mutation produces zero original compiles/links (Make control 0.235 s); shared ABI mutation rebuilds affected native/original objects (0.297 s). All 41 CVPACK, 45 C2 and 40 LINK original dependency rows retain the common wrapper and exclude native remaps/win32. Five period C/C++ modes pass; four MSVC code sections are identical, Watcom C compile passes. Eight native preprocessing and six runtime groups pass. One initial rebuild is expected. |
| Explicit progress file assertions | At a genuine App frontier, opt-in --partial-output-files-only captures declared Partial paths and their observed absence while allowing the specified unasserted extras. Raw artifact export remains complete; default and normal-exit final inventory/policy stay unchanged. | Five focused controls pass. Actual WLIB OpenFileCache to ReadImportData: 89 matching entries, original 742.802 ms, native 12.759 ms (setup 7.737 ms, execution 2.055 ms, checks 2.967 ms). Three meaningful logical outputs are checked; the internal original temporary file is retained in raw export. No PID binding, filename normalization or publication-only recapture. |
| Resolve MAP publics only when needed | Numeric graph metadata and object-bound rebuilt extents must not invoke the MSVC publics parser on a Watcom MAP. Resolve publics only for actual rebuilt_symbol metadata leaves or rebuilt code chunks; preserve paired inputs and extent hashes. | Eight projection controls pass. Current WCC export is byte-identical in 11.693 ms; full 77-function graph and 70 ordinary App RET checks pass in 90.189 ms. Canonical export parity passes in 11.998 ms. No original run or new MAP parser. |
| Exact CF command-line input | Retain the helper's actual pre-CreateProcessA text separately from the observed payload _LpCmdLine tail, verify existing native getcmd consumes that exact tail, and bind the existing optional fixture input. No argv reconstruction or new provider/protocol. | Seven Python controls and one focused Rust test pass; the VC1 NT helper compiles. Actual WPP parseCmdLine reaches CBanner with 92 matching entries: original capture 2,350.712 ms, native 9.014 ms. Full loader command text and payload tail remain distinct; no publication-only replay. |
| Exact raw Windows command-line input | Retain the actual CreateProcessA text separately from projected argv, bind mutable native storage through exit callbacks, and quote only arguments that need escaping. Existing raw captures remain unchanged. | Four focused Rust controls and two native input/lifetime controls pass. Actual WLINK GetCmdLine reaches NewCommandSource with 42 matching entries: original 1,217.919 ms, native 15.263 ms. Corrected WLIB seed exits 0 in 1,265.091 ms with exact unquoted switch/archive text; the old quoted failure remains preserved. |
| Ordinary CF debugger and controller retirement | Execute original CF instructions/CRT/gateway under Wine, mask recovered CODE envelopes, restore registry code/data, and collect real App entry/frontier traps. Remove Gate/Session/Capstone and old instruction-specific controllers/tests. | WCC GetEnvVar to SizeToClass: four matching entries, original 1,374.865 ms, native 5.716 ms. Execution-phase wall 2.106 ms, setup 1,226.292 ms; 2,559 RPCs; frontend CPU 420.694 ms, waited-child CPU 312.027 ms. 62 Rust, five mask/wire, 19 format and 19 native IPC controls pass; all three migrated CF profiles project through the common exporter. Old failed observations remain preserved. |
| Exact staged self-image input | Watcom resource code opens its actual staged argv0. A readonly source plus CC-only raw ranges freezes the exact masked file and recreates it as an owned T: sibling outside cwd, with unchanged-byte verification. | 57 baseline Rust tests and four focused recipe controls pass. Actual WLIB self-open reaches FindResources: original 1,269.207 ms, native 8.225 ms (setup 5.119 ms, execution 1.504 ms, checks 1.601 ms); exact 62,738 B file and 3,858 B overlay preserved. |
| Observed Watcom PE main arguments | Watcom startup replaces command-line argv0 with the staged module path. Bounded reads at the existing main entry retain requested and actual vectors separately and project observed args into native IO. | Three argv-specific controls and existing 53 Rust tests pass; no broker change. Actual WLIB main/hook/InitMsg to OpenResFile pair passes: original 682.673 ms, native 5.320 ms, four matching App entries. The subsequent exact staged-image input release qualifies its actual self-open. |
| Explicit nonreturn DIRECT terminal | Genuine target ExitProcess from a typed callback must retain its status and cleanup trace. Actual callback entry is observed through an owned breakpoint; pre-callback/wrong exits and returning callbacks fail the explicit contract. | 53 Rust tests and five authored controls pass (0.489–1.224 s per capture), plus native pairing/return rejection. Actual CL handler original exit 4 and 59 App entries pass in 899.556 ms; matching native passes in 7.961 ms. No post-exit helper reads or atexit machinery. |
| Finite WPP CF geometry and reached ENTER frame | Separate WPP identity/BSS/handoff size from WCC, wire RECON_CF_PROFILE into Make, and validate the actually reached 32-bit zero-nesting ENTER frame. | `b277c67f7`; 51 Rust tests, native ENTER88,0 execution/negative controls, existing gate/startup/environment and incremental Make checks pass; release build 17.26 s. WPP first-frontier qualification remains separate. |
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
| Typed WCHAR16 _wcsupr and bounded VC5 ASCII mutation | Bind the genuine import, then support its authenticated default-C integer path under explicit510 guards: validate the whole ASCII span before mutation, preserve the input alias/NUL/tails, and deny other profiles/states. | `ca070c16a`, `576e772ac`. Four GCC/Clang groups pass (typed build/check wall 1.551 s; success 1.514 s). Success checks cover 131 spans, three error tuples, empty/live108-byte FP, alias/tails/guard pages/tables and no partial write (12.0/11.3 ms); generic denials remain. RCDLL reuses original0129 and matches frontier bbe0 with 125 App entries: rebuild 0.653 s; native setup 74.825 / execution 36.180 / checks 68.827 ms (179.832 ms total). Independent review passed. Original per-call pointed bytes/errors/FP remain unmeasured; no API-only original or shared App credit. |
| Retained compiler symbols and reached console/ASCII boundaries | Keep a real instrumented destructor emitted; support witnessed explicit510 headless CONOUT$ failure, explicit510 default-C ASCII hex classification, and Microsoft generic/510 ASCII _mbsnbcpy without changing the binary’s profile. | `587bae883`, `e2350cac5`, `aef6c4dc2`, `b5c487eec`. RETAIN is native used / original empty: compiler/object checks take 0.257 s plus a corrected instruction-only audit 0.193 s; no App acceptance is implied. CONOUT$ supports only the exact RDWR/share0/OPEN_EXISTING tuple with default handles, NUL stdin, regular outputs and no controlling terminal: INVALID_HANDLE_VALUE/LastError6, preserved errno/DOS; four groups pass in 1.544 s and CL’s unchanged-original 100-byte adoption passes in 7.703 ms (`00e50255c`). No console success/query/close support. Hex mask128 returns128 only for 0–9/A–F/a–f under explicit510/default-state guards; four groups pass in 3.029 s, with RCDLL saved138 adoption 195.117 ms (48.976 ms execution; rebuild 0.718 s). _mbsnbcpy validates the count-bounded ASCII read span before copy/NUL-padding, preserving alias/errors/FP/tails; Watcom/unknown domains deny. Six groups pass in 3.375 s; saved CVPACK133 matches exit1, 70 entries and 94-byte output in 15.597 ms (setup3.822/execution9.595/checks2.180; rebuild0.862 s). Independent reviews passed. Provider analysis does not establish unmeasured original per-call state; no API-only original captures or shared App credit. |
| Private demangler/comparison typing and closed-file deletion | Keep authentic unvisited LINK CRT imports from escaping to host libc; let the reached VC1 LINK cleanup delete its real closed/unmapped output files. | `f3113ba55` binds __unDName’s six cdecl arguments/callback types; `451f1f2d1` binds SDK _mbsnbcmp. Both always deny125, including NULL/count0; no callbacks, demangling/comparison success or App credit. Four GCC/Clang generic/510 groups pass per slice in 1.679/1.800 s build/check wall (checks 30.6–67.4/25.6–31.3 ms). `ecb2b3422` implements actual DeleteFileA unlink for writable regular files after per-file View device/inode and current-process fd checks; unrelated live mappings remain allowed. Missing leaf/parent and read-only/access errors return FALSE with LastError2/3/5; CRT errno/DOS are preserved. Symlinks/directories/open-file/share/delete-pending domains remain unsupported. Four focused groups pass in 1.697 s, cached mapping controls in 1.662 s. Saved-original LINK adoption takes 29.113 ms; its required fresh completion pair takes original 1.149 s/native 30.072 ms (setup 8.298/execution 2.918/checks 18.855; build 4.603 s), matching Exit1, streams, trace, retained 2MiB data and deletion inventory. Independent reviews passed. Original per-delete BOOL/errors were not observed; ignored returns/no later GetLastError limit the actual claim. No API-only original campaign. |
| Reached wide comparison, Watcom binary open and public IMAGEHLP staging | Support the real RCDLL keyword comparison and WLIB resource self-open; preserve CVPACK’s named checksum branch instead of skipping its App file-size/mapping calls. | `_wcsicmp` typed `3c1a815fc` and success `a5440f4f6` support only explicit510/default-C ASCII comparison prefixes; generic/unknown domains deny. Four compiler/profile groups take 1.791/1.660 s; saved0143 native adoption takes 187.947 ms (execution 38.263 ms; rebuild 2.267 s), target unit 143.956 ms. Original per-call state remains unmeasured. Watcom typed `d9140bff5` and success `974310433` isolate flags 0200 and use genuine readonly binary descriptors for existing regular ASCII paths; unknown/failure states deny, with no absent-vararg read, FILE26 or numeric-fd/error claim. Typed checks take 2.199 s; six compiler/profile/FP success groups take 3.114 s. The exact masked-self-image pair reaches FindResources: original 1269.207 ms/native 8.225 ms (setup 5.119/execution 1.504/checks 1.601), readonly 62738B input unchanged. IMAGEHLP `69e2f1106` gates the named WINAPI4 export on LoadLibrary, but checksum invocation and unload still deny125; no provider hash gate/loader. Four groups take 1.622 s and actual CVPACK139 reaches map_file_region in 32.532 ms (execution 7.310 ms; build 10.113 s), without invoking the API. Independent reviews passed; no API-only original campaigns or shared App credit. |
| Typed heap growth, Watcom seek and mapped PE checksum | Keep unvisited HeapReAlloc typed and denied; support real normal resource seeks and CVPACK’s public checksum operation without changing target allocation or loader behavior. | `ef702a3a6` adds SDK WINAPI4 HeapReAlloc with unconditional125 only; four compiler/profile groups pass in 1.609 s, with no growth-success claim. `657661533` maps Watcom lseek to the existing regular binary descriptor backend for origins0/1/2 and nonnegative signed32 results; unknown/failure states deny, Microsoft mappings remain unchanged. Six groups pass in 3.100 s; genuine WLINK resource scanning reaches InitResources (original789.344/native13.342 ms; build1.530 s), and WLIB adoption passes original836.569/native9.564 ms (setup5.875/execution1.862/checks1.827). `c63eea9e6` computes the standard PE32 WORD checksum for a live registered mapped span, returning the exact NT-header alias and old/new sums through aligned disjoint external outputs while preserving image/tails/errors/FP; unsupported domains deny before writes, unload remains unsupported. Four groups pass in 1.823 s, covering the actual18320B/0x4b2a oracle, nonzero old checksum, odd length, carries and16 no-write denials. Actual CVPACK140 exits0 with independently correct raw checksum: original622.530/native17.700 ms (setup6.046/execution7.771/checks3.883; build/audit10.397 s). Independent reviews passed; original per-call API effects remain unmeasured. No API-only original campaign, FILE26 support, provider framework or shared App credit. |
| Watcom tell, binary mode and nonzeroing fixed allocation | Bind the real resource callback table and startup descriptor-mode calls; support the reached DOS gateway’s normal LocalAlloc0 request through existing allocation ownership. | `74a85ac54` maps Watcom tell(int) to the reviewed lseek(fd,0,SEEK_CUR); six groups pass in 3.260 s with unchanged position/errors/full FP. WLIB’s callback-table pair reaches WResReadDir (original1075.4/native9.10 ms); tell is address-only at that frontier, not invoked. `490035b39` accepts Watcom setmode0200 on open shared descriptors, performs the real existing binary transition and translates previous binary8000/text4000 to0200/0100; generic/text-input/unknown domains deny. Six groups pass in 2.977 s; WLINK’s two real stdin/stdout calls reach InitNodes (original1317.042/native18.507 ms; setup11.888/execution2.267/checks4.352; build1.542 s). A test-only descriptor snapshot was corrected from64 to actual256 slots before the passing checks, with runtime bytes unchanged. `add8756e9` accepts positive LMEM_FIXED0 using malloc under existing pointer/size/flags records; ZEROINIT and same-owner free remain unchanged. Six groups pass in 3.169 s, checking exact size, distinct ownership, test-owned nonzero contents, errors/full FP and5 denials; old zero-init cached references pass GCC/Clang in1.537 s. Actual WCC/WPP64KiB LMEM_FIXED0 adoption now passes at AddBlock: WCC original1608.500/native7.708 ms (setup4.089/execution3.130/checks.489), WPP original1.332 s/native5.130 ms. Initial bytes are unspecified and no original zeroing, per-call errors/FP or release observation is claimed. Independent reviews passed; no FILE26/new backend, allocator tuning or API-only original campaign. |
| Watcom registration-only signals | Preserve the reached WLIB main registrations through four independent typed handler slots, without installing Linux signal or console handlers. | `3e2bab303` supports only explicit Watcom numbers1/4/6/7: initial old tokens2/1/1/1, then exact old-handler return/new-handler store, including IGN1/DFL2. NULL/ERR3, other numbers, raise and delivery remain unsupported. Six compiler/profile groups pass in3.149 s (slot independence, callback identity, errors/full FP, unchanged host dispositions and8 denials); existing typed-surface controls pass GCC/Clang in1.468 s, with the former SIGINT denial correctly moved to unsupported SIGFPE. The genuine22-entry WLIB continuation through four registrations and normal setjmp to InitToolState passes original1168.261/native8.393 ms. Independent review passed; original per-call API effects and private thread/raise-pointer bytes remain unmeasured/unmodeled, with no delivery or API-only original claim. Subsequent WCC/WPP fixed-allocation adoption is recorded in the preceding allocation row; no release or zeroing claim is added. |
| Cold CRT imports, Watcom storage and reached text comparisons | Link authentic unvisited imports without host fallbacks, preserve encountered CRT storage/flat-pointer spelling, and support the bounded text conversions actually needed by current callers. | `7f2148b0d` adds Watcom-only typed getcmd/memicmp denial boundaries; four groups pass1.607 s. WLIB help reaches PrintUsage (original1246.693/native8.230 ms) without getcmd, and WLINK reaches InitSymStorage (698.240/15.197 ms) before the memicmp assignment. `a198cfe28` binds the private static cdecl3 NameMap::open directly to a typed125 symbol with no wrapper; four C/C++ groups pass1.711 s and C1’s actual unvisited-import pair passes721.122/19.070 ms. `cbb5a87bb` provides Watcom unsigned32 _amblksiz initialized to authenticated16 and the native-only flat-DS __far annotation, without host allocator policy or general selector/segmented-storage support; four groups pass1.618 s, WCC main→CMemInit passes1046.200/5.518 ms (build.903 s). `74f8ad0e5` reuses signed32 decimal _itoa for Microsoft _ltoa, denying other radices/Watcom; _gcvt remains typed125. Six groups pass3.127 s; C1XX’s0.987 s/28.269 ms caller pair takes the string arm before either conversion, so numeric runtime adoption is unobserved. `5f757e803` supports default-C/CP0/MB1/gate0 ASCII _mbsnbcmp prefixes with normalized unsigned ordering; six groups pass3.331 s and LINK’s unchanged-original191 native adoption passes25.972 ms (setup11.824/execution4.053/checks10.095; build.671 s). Watcom/unknown states deny; canonical strncmp pre-scans its left prefix, so no identical read-access tracing or unread-tail claim is made. Independent reviews passed; original per-call effects remain unmeasured, no API-only original campaign or new numeric/provider framework. |
| Watcom cold console and stream spellings | Link complete period-C functions while keeping unvisited console and buffering operations closed. | `6b1290aa8` adds native conio.h and Watcom-only int getch(void), always125. Four existing cold-surface groups pass1.717 s; WLIB’s regular-stdout PauseOnConsole→FiniToolState pair passes original1340.634/native10.767 ms without invoking getch or the console branch. `b5cb4c994` supplies stdaux/stdprn pointer spellings for existing iob3/4, bare isatty to the descriptor boolean helper, and correct Watcom _IOLBF0200 with a distinct typed-denied setvbuf boundary; Microsoft40 mapping remains unchanged. Four groups pass1.751 s (typing/profile isolation/denials/no writes), and WCC’s first FrontEnd→InitGlobalVars frontier passes978.197/5.240 ms (build.8135 s) before any stream operation. Independent reviews passed; no stream initialization/close, FILE26, console input or buffering success is claimed. No additional original or test was run for this report. |
| Reached Watcom stdout line buffering | Retain WCC’s supplied 128-byte buffer and flush on newline, capacity and explicit close/flush through existing lowio. | `0d7c3de15` replaces the earlier typed-only setvbuf staging for explicit Watcom fresh nonterminal stdout, mode0200 and an owned128B buffer; a separate line state preserves caller ownership without using Microsoft string-stream flags. Six compiler/profile groups pass in3.292 s, checking alias/tails, CRLF visibility, capacity/explicit/close flushing, errors/full108-byte FP, unrelated hostfd3/4 survival and9 denials. WCC reuses its original849.112 ms reference and matches14 entries at InitMsg in5.779 ms (native build.495 s); this real pair qualifies setup, while focused checks cover newline behavior. Independent review passed. Blank auxiliary/printer slots retain generic ignoredEOF without host close/free; original close-error effects, FILE26 and reconfiguration remain unclaimed. No additional original was run. |
| Repeated stdout buffers, exact command input and cold DEF APIs | Preserve WPP’s real buffer replacement and expose WLINK’s actual raw command text without reconstructing it from argv; keep unvisited DEF services closed. | `d6358e5bf` adds fresh256B stdout and disjoint empty256→256 user-buffer replacement, tracking capacity without flushing/freeing the old permanent buffer;128B replacement and pending reconfiguration still deny. Six groups pass3.504 s; saved-original WPP adoption reaches SrcFileFClose in5.546 ms versus original1.692 s (native build.524 s). `a8adc9169` binds exact mutable ASCII command text once and returns its alias through GetCommandLineA; Watcom getcmd skips the first whitespace-delimited token and copies the literal remaining tail, preserving quotes. Six groups pass3.428 s (alias, guard-page/tails, errors/full FP and16 denials); shared runner `61b2f237a` now supplies exact raw input, and the actual42-entry WLINK GetCmdLine→NewCommandSource pair passes original1217.919/native15.263 ms, with raw R:command text distinct from observed T:argv0. Unbound input denies; no argv reconstruction or quote decoding. `00f3d55fd` adds SDK WINAPI GetTempPathA/GetTempFileNameA/SetEnvironmentVariableA with unconditional125 only; four groups pass1.691 s with9 no-access/no-write controls. Independent reviews passed; no temporary/environment success, FILE26 expansion or additional API-only original. |
| Reached classification and Watcom compiler/path/file semantics | Close actual accelerator, type-initialization, filename and file-open boundaries through the existing native state/descriptor backend. | `f0d8a1ec9` supports explicit510/default-C ASCII iswctype107 with exact1/2/4/0 table-AND results;103/generic remain denied. Four groups pass1.610 s, saved RCDLL adoption185.153 ms. `df20b369a` supplies Watcom DWORD fill/endptr without register emulation; six groups pass3.438 s, WCC CTypeInit pair1545.232/5.699 ms with the shared-C3 seed explicit. `ba70f19e0` packs requested splitpath2 components for relative ASCII leaves<260, preserving NULL selectors/pointers; six groups pass3.295 s, WASM adoption8.338 ms. `ea2632e72` separates volatile Watcom errno/ENOENT1/EMFILE11 from host errno and provides startup char __OS2; real missing readonly paths return-1/target1, other failures stay closed. Reused six groups/two surface controls pass7.458 s, WLINK saved adoption538.194/13.459 ms. `1675b05ce` translates only writable0162+0666/0262+0600|0666 and target flag spellings to real regular relative-leaf create/truncate text/binary descriptors. Six groups pass3.109 s and readonly controls1.375 s; WLIB text/binary pairs696.673/8.546 and1077.102/11.156 ms, WASM saved binary pair1486.599/11.291 ms. Independent reviews passed. Errors/full FP/tails/unknown-domain controls remain bounded; no FILE26, thread-layout, OS-detection, original per-call error claims or API-only original campaign. |
| Watcom file positions, null-input getch and cold numeric typing | Use existing registered stream/descriptor operations for actual normal file-position and EOF input paths; retain an unvisited numeric import. | `e2f4537e7` supplies source-visible4B fpos_t and guarded fgetpos/fsetpos over existing ftell/SEEK_SET; six groups pass2.995 s. WASM first fgetpos continuation passes original768.494/native11.264 ms; saved0 is statically derived, fsetpos remains unobserved. `ba87eca6e` admits only registered /dev/null stdin in initial text or qualified binary mode, using real ReadFile into initialized0 and requiring count0 without lowio EOF changes. Six groups pass2.998 s; WLINK reuses original1139.745 ms and matches full8475B helptext/trace to Ignite in17.616 ms (setup8.459/execution5.096/checks4.061). `95988d6ef` adds typed Watcom utoa with unconditional125/no access; four groups pass1.491 s and WLIB compile-only continuation passes632.788/8.438 ms without invoking conversion. Independent reviews passed. Unknown streams/failures, console/other input and numeric success stay unsupported; no FILE26/private console-mode or original per-call error claim, and no extra original for publication. |
| Reached VC510 private-heap cleanup | Complete RCDLL cleanup by retiring its exact registered heap and all outstanding owned allocations. | `7089c9add` checks live ownership before access, releases every tracked block mapping and then the owner, and returns TRUE only after successful retirement. Six GCC/Clang groups pass3.082 s, covering independent surviving heaps, empty owners, mapping retirement, errors/full108B FP and five scope denials; existing generic heap controls also pass. RCDLL reuses its successful839.197 ms original: native completes exit0 in203.741 ms (setup76.448/execution57.043/checks70.249; build.530 s), matching217 actual first entries and the exact3672B RES SHA ad4feb23…9318. Native cleanup owns4358 live blocks/157256 requested bytes; original private inventory and per-call errors are unmeasured. Independent review passed. Other profiles, NULL/unknown/dead owners and release failures deny125; no general stale-handle generation guarantee after later address reuse or API-only original campaign. |
| Watcom live nonlocal return and existing-directory output | Resume the reached active Spawn context and create archive extraction output through existing relative directories. | `ce8c8d4df` admits value1 only for a matching saved52B Watcom context; the caller must guarantee the same thread, still-live frame and same unwind chain. Existing libc context is reused without target-byte/x87 serialization or trace-depth state. Six groups pass3.035 s, checking opaque owner/canaries, current FP/errors and seven scope denials. Actual WLINK Suicide→Spawn cleanup passes original1139.107/native19.893 ms with full8475B helptext/trace to CloseSpillFile; Windows RtlUnwind remains masked. `d429066b3` extends exact writable tuples through translated existing nonsymlink directory components and a regular/new nonsymlink leaf, rejecting traversal/absolute/drive/UNC/empty/trailing-dot-space paths and creating no directories. Six groups pass3.452 s with complete inventories, slash/nested variants and18 denials. Actual WLIB creates out\keep.obj and reaches ReportDuplicateSymbol in971.691/10.777 ms (setup5.851/execution2.735/checks2.191). Independent reviews passed; no expired-owner/SEH, new flag framework or API-only original campaign. `e51332918` adds only the Watcom bare itoa alias to existing signed32 conversion; four typing/isolation groups pass1.477 s, with actual decimal invocation still unobserved. |
| Reached Watcom archive-offset hexadecimal conversion | Format the archive listing offset with the existing unsigned32 numeric backend. | `66c33e3ff` admits explicit Watcom utoa radix16, returning the supplied start pointer with the verified lowercase alphabet; the caller owns a valid12B buffer. NULL/wrapping spans, other radices and profiles remain125. Four reused GCC/Clang typing/numeric groups pass1.484 s for zero/64/high-bit/max values, output boundaries, independent errors/full108B FP and retained denials. Actual WLIB heading/output continuation matches original1380.164/native15.345 ms (setup9.357/execution3.144/checks2.844; build.983 s),153 entries to PrintModuleSymbol. Independent source review passed; no new conversion backend, capacity-ownership framework, per-call original error claim or API-only original. |
| Watcom exclusive temporary-file creation | Create WLIB’s absent temporary archive through the real exclusive-create descriptor backend. | `7adf125e5` admits exact0621/0666 for an absent ASCII relative leaf, mapping WRONLY1/CREAT20/BINARY200/EXCL400 to existing MS8501 atomic CREATE_NEW; normal close and0262 reopen retain shared ownership. Existing paths/collisions, directories for this tuple and unknown failures deny125 without truncation. Six reused writable groups pass3.560 s, readonly two-argument controls1.516 s, and GCC/Clang host-flag isolation checks pass. Actual WLIB create-close-reopen continuation matches original1011.978/native10.718 ms (setup5.883/execution2.616/checks2.219),87 entries to getcomdat. Independent review passed; prior tuples remain unchanged, no collision-result/per-call original error claim or API-only original campaign. |
| VC510 whole-owned arena decommit and release | Preserve LINK’s exact arena reservation between whole-allocation decommit and final release. | `5984410f3` admits MEM_DECOMMIT only for an exact tracked base/full length from VirtualAlloc(NULL,n,COMMIT|RESERVE,READWRITE). Checked PROT_NONE protection and page discard retain the owner/reservation until ordinary zero-size MEM_RELEASE; partial/foreign/ineligible/repeated decommit and recommit remain125. All View construction paths initialize the native state, and other profiles/release behavior remain unchanged. Six GCC/Clang profile groups pass2.960 s for retained inaccessible/discarded mappings, retirement, independent allocations and errors/full108B FP; existing top-down/mapped controls pass1.468 s. LINK reuses its case272 original (1557.741 ms) and matches265 entries to58b80 in31.674 ms (setup12.387/execution8.836/checks10.451; build4.326 s). Independent review passed; no general VM model, per-call original error claim or API-only original campaign. |
| Identified Watcom1994 qsort equal-key order | Correct WLIB’s visible equal-name module order without changing its comparator or input. | `cbf2c1b5a` adds readable midpoint/forward partition sorting with the actual count2 and equal-tail behavior, selected only by Watcom plus RECON_QSORT_WATCOM_1994. The later shipped hybrid is a different variant; existing Microsoft and other Watcom dispatch remain unchanged. Eight GCC/Clang profile groups pass3.061 s for counts0/1/2/3/7/8/9..240, duplicate/all-equal order, unaligned widths, complete permutation/tails, current FP and real callback effects; bad opt-in and five span/callback domains are rejected. WLIB reuses original1260.568 ms: native19.888 ms (setup5.591/execution11.354/checks2.943; build1.028 s) matches188 entries and the complete15343B listing SHA3913a42a…43ff3, including keep-before-addone equal keys. Independent review binds actual468B algorithm/38B swap and input240 pointers. No App CRT credit, host interposition, output mask/tie breaker or API-only original. |
| Watcom internal temporary-cache identity and creation | Supply a real native unique name only where App treats it as an internal, deleted, non-name-sensitive cache identity. | `ad369374a` uses a short relative RCXXXXXX host-CRT name and genuine existing-access missing-leaf check, creates no file, and returns the valid13B caller buffer alias with tails/errors/FP retained. Exact0261/0666 delegates existing MS8301 creation for an absent relative leaf; normal0200 reopen/unlink remains shared. Six GCC/Clang profile groups pass2.998 s for no-create,16 unique names and real cache lifecycle; prior readonly/writable controls pass1.715 s. Actual WLIB OpenFileCache→ReadImportData matches89 entries in original742.802/native12.759 ms using explicit selected meaningful Partial outputs (tool e495bd9f8); full raw original artifacts remain retained and final-exit full inventory stays strict. Independent review passed. Cleanup is not yet observed in this prefix; no historical AAA/A..P or PID identity claim, NULL/static-result/unknown-filesystem support, concurrent namespace mutation or API-only original. |
| VC510 module-name _mbscmp identity | Use the actual case-sensitive import discovered by LINK’s complete IAT audit. | `89e030e86` supplies the SDK-typed boundary for explicit510/default-C/CP0/MB1/gate0 ASCII, reusing the existing normalized unsigned comparison through mismatch or NUL. Other profiles, NULL, nonASCII compared units and unknown state deny125. Six GCC/Clang profile groups pass 3.125 s, including case sensitivity, readonly boundaries, independent errors and full FP retention. Corrected LINK reuses saved304: status0, full entry order and final hello.exe SHA match in 29.687 ms (setup14.061/execution8.500/checks7.127; build4.449 s). Independent review passed; no original live per-call state, identical read-schedule or general multibyte claim, and no new original capture. |
| VC510 current-directory temporary-file APIs | Create LINK’s real response file for its DLL import-library path. | `9baa4de08` admits bound target TMP=".", verified canonical cwd and260B output for GetTempPathA; GetTempFileNameA admits that directory, lah/unique0 and disjoint260B output, creates/closes a genuine exclusive lah%04x.tmp file and retries collisions. Six GCC/Clang profile groups pass3.108 s for path/number agreement, empty creation, collision preservation, reopen/write/delete, buffer boundaries, independent errors/full FP and twelve denials. Actual LINK reuses its original Reserve frontier:183 entries match in35.185 ms (setup16.750/execution3.584/checks14.850; build.315 s) after binding the existing public CRT argument/environment namespace. Independent review passed. Each side retains its own generated identity; original pathname text/success LastError and later SELF/write/delete remain unclaimed. Other profiles and input domains deny125; no API-only original. |
| Cold Watcom directory SDK surface | Compile WLINK’s complete WildCard body while the actual control1 path calls its App callback. | `c7547a676` supplies the packed289B NT DIR/dirent layout (attribute21/name30/first288), attribute constants and profile-only opendir/readdir/closedir aliases to typed no-access125 boundaries. Host/shim DIR remains separate. Six GCC/Clang C profile groups pass3.062 s for SDK typing and NULL/owned/invalid denial; six C++ typing/host-isolation groups pass1.176 s. Actual WildCard→AddRuntime matches original1196.750/native17.974 ms (setup9.795/execution2.654/checks5.524; build1.143 s, scoped graph/RET22.434 ms). Independent review passed; no directory API was invoked or enumeration/allocation/close behavior qualified. WPP retains its approved scoped directory adapter after including shared headers first. |
| VC510 counted ASCII MacRoman conversion | Let RCDLL validate page10000 and convert its actual menu strings, including the separator’s invalid count0 calls. | `4661ae044` admits IsValidCodePage10000 and flags0/NULL replacement+used counted ASCII query or disjoint exact-count output, with full prevalidation and no added terminator. The reached count0/cap0/nonNULL aligned input returns0/ERROR_INVALID_PARAMETER87 without pointed access; this is failure, not empty conversion success. Six GCC/Clang profile groups pass3.114 s for buffers/readonly bounds/no-partial-write, errors/full FP, protected zero-count pointers and14 denials; other pages/reverse conversion stay unchanged. Saved0017 original711.961/native15.196 ms (setup7.293/execution6.168/checks1.735; build.542 s) matches the dialog-count frontier; earlier positive0016 also passed. Independent review passed; original per-call pointed bytes/errors/FP remain unmeasured, and nonASCII/other profiles remain unsupported. No API-only original. |
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
| Opt-in PE same-EXE child capture | `--progress --pe-self-child` supervises one synchronous same-file child with separate process bases and breakpoint bytes, shared first-entry order, full DWORD statuses and owned cleanup. Default capture remains parent-only; native child frontier remains a fixture failure pending standalone qualification. | Six wire/CLI, five reference and two typed tests plus eight existing native SELF modes pass. The authored successful child pair takes original 1,864.169 ms / native 13.401 ms; saved final native replay 16.688 ms. Child frontier cleanup takes 789.152 ms, different-file child rejects before entry, and default behavior is retained. The first failed authored attempt is preserved. No real LINK or publication-only original run. |
| Optional exact target launch environment | Retain the actual pre-CreateProcessA OS block and provider identity in capture.bin; stage it independently of CRT/host environment and pass the accepted SELF request vector to fresh children. Optional trusted policy fields expose the saved initial baseline; defaults remain empty. | Five focused Rust controls, eight native SELF modes and actual root/SELF staging pass (16.280 ms). Real supervised LINK records 199 entries and child frontier 188a0 in 1,258.133 ms. Corrected native policy validates the baseline, one insertion and 185 response bytes, then honestly fails at the pending child in 57.378 ms (200 entries including parent fatal). This is discovery/transport qualification, not combined acceptance. No publication-only original run. |
| Clear completed-capture partial expectations | A completed original capture clears inherited frontier-only expectations instead of carrying them forward from its input capture. | Root one-line correction to original.rs; C1XX encountered and explicitly finalized the real completed case without recapturing. Formatting and all 32 Rust tests pass. |
| Qualified nonexact CRT identities | Permit CVPACK's explicitly reviewed original CRT variants to be classified as CRT while retaining identity-only graph projection, exact existing hashes/bounds and zero body availability/application credit. | `1e10445a3`. Two focused tests, formatting, Clippy and five CLI rejection controls pass. The actual 187-function audit takes 213 ms and its report is byte-identical to the old baseline. CVPACK mask verification restores only the three approved CRT bodies (1,253 bytes), retains all 20 App traps, and ten native cases/unit executors pass in 161.8 ms. Consumer correction is committed in `d7264dd14`; final audit passes in 4.708 s including original-compiler prerequisite rebuild and identification. |
| Canonical pending CVPACK drivers | Validate pending App identities, canonical extents, text-data and uniqueness while retaining stub status, no body availability or acceptance, and zero App credit. | `c2739c352`. Three adapter tests, focused rejection controls, formatting and Clippy pass. The frozen 208-function report is byte-identical (225 ms); an explicitly derived 207-plus-pending view passes (231 ms). No speedup claim. |
| Original-only CRT instruction facts | Export explicitly selected, hash-bound canonical CRT spans through the existing decoder without inventing a paired NT body, checked ABI, execution permission or App credit. | `307e3ed37`. Seven instruction-export tests, 15 existing paired controls, 14 focused support controls, formatting and Clippy pass. The actual WCC startup/CMain sample has two owners and 53 instruction rows (about 19 ms); the offline consumer verifies bytes, relocation and null checked cleanup. Default and paired CF reports remain byte-identical. No capture or speedup claim. |
| Explicit bodyless endpoint correlation | Normalize a finite, evidenced Unknown endpoint against its real MAP entry without adding a body, extent, classification, acceptance or execution permission. | `1a2437baa`. Three registry tests, ten actual denial controls, formatting and Clippy pass. WCC's two-owner static graph passes in 26.4 ms; unchanged-policy reports retain parity. The live WCC audit also passes. No capture or speedup claim. |
| Rebuilt funclet ownership projection | Carry exact symbol-relative rebuilt code chunks from finite policy into the existing driver/ownership checks; original identity, extents and status remain unchanged. | `0e0030058`. Six projection tests and focused malformed-property controls pass. Actual LINK export takes 26.8 ms; its 161-vertex graph and 153 scoped RET checks pass in 431.7 ms, with the separate normal/action/handler role residual retained. Canonical export/report parity passes; unrelated pre-existing strict Clippy warnings remain logged. No capture or speedup claim. |
| Empty Watcom DATA boundaries | Preserve exact zero-sized DATA section-end markers as MAP provenance without creating callable publics or widening the mapped image. | `8c8f2c818`. Three parser tests, malformed-marker controls, formatting and Clippy pass. Actual WCC static audit passes in 35.8 ms; independent review takes 28.3 ms with the same report. Prior reports retain byte parity. No capture or App credit. |
| Object-bound rebuilt body ends | Describe WLINK's actual compiled contribution end and separately verify its one zero alignment byte, binding the current PE, MAP, object and witness. CFG/RET and adjacent ownership stay strict; no source padding or fake text-data. | `979a9f11e`, `45b58e6c4`. Four graph-registry and seven projection tests pass; ten stale-input, endpoint and padding-flow controls reject. Canonical export takes 12.2 ms; four App graphs and three scoped RET checks pass in 18.1 ms with report parity. Independent review passes in 19.4 ms. No capture or speedup claim. |
| Paired compatibility CALL boundaries | Exact witnessed CALL sites can share an opaque boundary role across the authorized DOS-gateway/Win32 adaptation. Raw instructions, unrelated graph edges, RET and ownership remain strict; no global provider alias or ABI grant. | `fa6e4f06d`. Three focused tests, nine rejection controls and absent-feature report parity pass. Scoped WCC/WPP checks take 24.7/32.4 ms; independent review passes. No target execution or speedup claim. |
| Exact shared RET ownership | Two canonical application identities may explicitly share only the child's sole `C3` at the parent's discovered terminal RET. Other overlaps reject; the actual caller edge stays intact and unique original code grows by zero bytes. | `58310fa82`. Two focused tests, 14 rejection controls, formatting/Clippy and default report parity pass. Targeted owner/entry and caller checks take 16.4/14.2 ms; independent review passes. No new capture, runtime permission or speedup claim. |
| Optional documentary pins | Canonical identity, CALL-boundary, shared-return and body-end declarations no longer require opaque diagnostic files. Supplied null/malformed/hash bindings still reject; actual object/image/MAP and interpreted CF-profile pins stay mandatory. | `81cc675fb`. Eight focused graph and seven exporter tests pass; old supplied-document reports retain parity. Documentary-free WLIB/WPP checks take 14.6/33.9 ms. No capture or runtime permission. |
| Exact shared terminal epilogues | Two real App owners may declare only their complete shared POP/POP/RET suffix. Both CFGs must discover every instruction at both chunk ends; identities, control paths and unique code-byte coverage stay intact. | `81cc675fb`. Actual WPP three-byte suffix passes in 35.5 ms; wrong bytes, starts, owners and undeclared overlap reject. Independent review passes. No new target execution. |
| Decoded shared terminal suffixes | Extends the same exact named-pair ownership rule to 1–32 decoded bytes, with contiguous fallthrough and an unprefixed near RET. Calls and other preceding control transfers remain rejected. | `2caa301ce`. Five focused units and Clippy pass; scoped WLIB seven-byte suffix passes in 20.83 ms (independent 20.00 ms). Wrong/partial/undeclared ranges reject; old three-byte, single-RET and default reports retain byte parity. No whole-case or execution-permission claim. |
| Exact shared CALL/return tails | Separate named-pair declaration verifies one explicit direct CALL to its canonical target, the complete shared tail and both raw caller edges. Branchless suffixes remain strict; no runtime permission or ABI inference. | `c60923351`. Seven focused units and Clippy pass; scoped WLIB/WPP positives take 14.28/39.54 ms (independent 15.59/40.06 ms), with 13 denial controls. Earlier suffix, single-RET and default reports retain byte parity. No whole-case or speedup claim. |
| Exact shared terminal jumps | Reuses exact named-pair ownership and decoded transfer roles for a shared terminal JMP, retaining both raw edges to a canonical endpoint. Pending targets stay bodyless; nonlocal-return proof remains binary-specific. | `3e5f784a7`. Nine focused units and Clippy pass; scoped WLIB positive takes 17.92 ms (independent 18.99 ms), with nine denials. Global RET still rejects the pending target, and prior reports retain byte parity. No execution permission or App credit. |
| Explicit CALL within a shared JMP tail | The existing exact named-pair declaration may bind one direct CALL before its terminal JMP. Both roles must be decoded and consumed in both owners; raw edges and the independent nonlocal residual remain intact. | `66d3737fa`. Eleven focused units (two new), Clippy and eleven denials pass. Scoped WLIB positive takes 16.57 ms (independent 17.68 ms); old JMP and CALL/RET reports retain byte parity. Strict RET still rejects the pending endpoint. No target execution, permission or speedup claim. |
| Exact shared terminal CFG regions | Verify the complete two-owner intersection across finite exact-byte regions against both existing CFGs. Explicit branches and direct/opaque CALLs retain their raw identities; closed acyclic normal successors end at real near RETs. No callback ABI, preservation or runtime permission follows. | `c96d4f78f`. Fifteen focused units (four new), Clippy and twelve denials pass. Strict scoped WLIB split-return and opaque-call checks take 22.06/20.15 ms; prior/default reports retain byte parity. No whole-case, unique-byte credit or speedup claim. |
| Finite terminating CALLs and unknown cleanup | A declared final E8 to an already-classified CRT process-exit endpoint retains its raw callee/noRET and suppresses only false next-owner fallthrough. Only that explicit owner can qualify as no-return through its existing closed acyclic CFG; tail callers retain real RET sets, while unknown/ambiguous/cyclic endpoints still reject. | `a130f3975`. Four focused units, Clippy, ten denials and default byte parity pass. Frozen WLIB qualification takes 482.93 ms for 241 App graphs and 234 ordinary RET checks; the independent exit residual stays mandatory. No fake RET0, per-caller exceptions, target run or speedup claim. |
| Finite virtual-call EDI preservation | Bind an exact receiver/vptr chain, slot and finite actual vtable targets to separately qualified normal-return ABI contracts. Retain only an existing EDI value; the virtual call remains opaque and pending targets gain no body availability or credit. | `da3929a19`. Two new and six existing units, Clippy and 17 denial controls pass; actual one-owner C1 audit takes 82.42 ms. Existing FF15 and absent-feature reports retain byte parity. No original replay or speedup claim. |
| Object-bound paragraph padding | Extend the existing exact body-end descriptor to 1–15 zero alignment bytes before the unchanged next public, preserving mandatory machine-artifact bindings and rejection of control flow into the gap. | `81cc675fb`. Actual WLINK nine-byte gap passes in 15.6 ms; metadata export takes 13.7 ms. Missing/stale bindings, invalid lengths/bytes and a root in the final padding byte reject. No source padding or speedup claim. |
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
| Offline comparison provenance | `capture derive-reference` binds untouched raw parent report/capture to a comparison-only fixture, records exact changed output rules and added harness asset hashes, and preserves all original observations. Native replay revalidates the chain; ordinary binding remains strict. C1XX Ledger actual adoption matches 74 entries and exit 0 in 37.258 ms with 154 initialized pointer DWORD ranges plus four original-uninitialized reserved WORD ranges; no original rerun. Formatting, 49 Rust tests, native provenance/tamper controls and existing reference/smoke checks pass. |
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
their current cases, then pause and report before moving beyond them. The user
later explicitly released CL and CVPACK to continue autonomously beyond a
single function or case; the other five unfinished tools retain the original
current-case scope. CL has now completed all identified application code and
joined RC and CVTRES in the finished, idle group. The table records reported
committed checkpoints, rather than intermediate function progress. Matching
current-case WIP remains excluded until the encompassing case is complete.
RCDLL has now completed its released rich-resource case and LINK its hello EXE
case; both are paused. C1, C1XX and C2 continue their current cases. CVPACK's
subsequent C6 case is unfinished after an agent-platform rejection; its source,
scoped patch, compiled artifacts and last valid pair are preserved in owned
scratch, with no retry through another worker and no additional completion credit.

| Binary | Committed App functions / bytes | Whole-text coverage | Reported committed checkpoint |
| --- | --- | ---: | --- |
| C1 | 159 / 33,030 | 10.25% | Minimal C: open_source_descriptor after the global-record writer and source-file lookup; `1d81b1477` |
| C1XX | 174 / 27,227 | 3.62% | Minimal C++: large writer and subsequent frontend work remain current-case WIP; `9cf5cc28d` |
| C2 | 209 / 51,805 | 9.04% | Numeric case: new_optimizer_context; `6ae5a84a2` |
| CL | 112 / 18,545 | 97.29% | Complete and paused: all identified App code; `f7613ff9c` |
| CVPACK | 168 / 48,556 | 77.49% | Thirteen cases complete, including the `/Zi` static-library archive and authored field-list continuations; `2a01eab26`, `0cf8cb702` |
| LINK | 304 / 96,662 | 23.30% | Hello EXE and bounded definition-buffer DIRECT prerequisite complete and paused; `89702eb2f` |
| RCDLL | 220 / 51,779 | 69.40% | Rich-resource case complete and paused; `f6cbd9acd` |
| RC | 2 / 277 | 19.26% | Complete: all identified App code |
| CVTRES | 22 / 7,445 | 76.80% | Complete: all identified App code |

CL has a separately supported 100% identified-App percentage; other unfinished
non-CRT denominators remain unclassified. Whole-text ratios include runtime,
data and padding.

The new CVPACK C++ growth case adds four functions and 730 code bytes. Its
120-entry original/native completion has exit status zero, empty streams and
an identical 285,168-byte packed DLL. The original takes 1.348 s; native replay
takes 51.107 ms (4.902 ms setup, 42.322 ms execution and 3.882 ms checking).
The complete original trace has no Widen/MSPDB traversal, so this case needs
neither the old input-specific oracle nor a new dependency framework.
The three cached cases, 13 units and strict audit pass in 0.193 s; the final
combined original/native build and audit takes 24.04 s after shared headers
force a full rebuild.

CVPACK subsequently completed a genuine CK1017 PCH diagnostic and a successful
CRT-free schema catalog. Both use actual original compiler output; no CodeView
data was fabricated. A duplicate `/min` flag was corrected to the single
original DWORD owner. Two independently proved unwritten source-index tail
bytes are zeroed natively and masked precisely under the documented defect
policy; every defined byte remains strict. Five cached cases, 13 units and the
original-build/strict-audit checkpoint pass in 2.020 s. The diagnostic's counted
copy adoption reuses its unchanged original and completes in 15.597 ms natively.

CVPACK has now also completed the telemetry workload and case `0006` using
`/RELEASE`. The latter exposed the real IMAGEHLP load/export and public checksum
dependency. Shared `69e2f1106` supplies the typed named route; `c63eea9e6`
implements the bounded mapped-PE32 checksum. The captured 18,320-byte input and
completed original independently establish checksum `0x4b2a`. Each output's raw
checksum is checked separately; comparison masks only three proved unwritten
source-index tail bytes and their dependent checksum DWORD, and asserts native
tail zeros. The finished case commits as `fed3a6c99`: original 622.530 ms,
native 17.700 ms, original/native build and audit 10.397 s, then all seven cached
cases, 13 units and strict graph/RET gates in 0.370 s. Publication reused the
identical tested source and pair without another original run.
Case `0007` subsequently completed a genuine VC1.0 `/Z7` two-TU archive through
C7 narrow-index conversion, not C6 legacy conversion (`d9eb4fded`). The real
25-byte `record_widen` wrapper uses a separately observed, case-bound typed
dependency response: four object lifetimes, 35 Record inputs and two symbol
Query/Emit spans. Its bounded observation ends at the final dependency return;
the distinct ordinary CLI pair establishes completion and the identical
5,216-byte output (original 1.333 s, native 10.433 ms). Eight cases, 18 units,
the original build and strict graph/RET gates pass in 8.548 s. The old
input-specific Widen oracle is not reused.

Case `0008` completes a genuine VC5 `/Zi` two-TU archive with its 53,248-byte
compiler PDB (`aaafcf21d`). Three App bodies add 1,839 code bytes. Existing
read/until/emit support observes the actual successful typed PDB exports and
19 mutable record spans totaling 532 bytes, retained through the real Close
calls. No PDB parser or new capture framework is introduced; successful Open's
unconsumed error output stays unclaimed. The final CLI pair produces an exact
5,208-byte output and leaves the PDB unchanged: original 688.717 ms, native
13.956 ms (setup 7.604, execution 4.515, checks 1.837 ms). Nine cases, 23 units
and the original build pass; the 45.479 s batch includes a shared Cargo build
wait. Required cached-import metadata uses the documented cdecl nonvolatile
contract; the corrected 144-graph/143-RET audit takes 0.315 s without another
execution. The harness-only PPB transcript then moves outside App cwd into
capture assets (`b83e321f3`); affected native replay passes in 10.719 ms.
Historical original reports remain intact, with no fabricated derived receipt
or publication-only original rerun. Other binaries' current-case working
implementations remain separate from the committed snapshots above.

Internal DIRECT cases `0009` and `0010` then add 17 App functions and 2,690
code bytes without claiming ordinary CLI reach. The cache case uses real
two-page/two-descriptor owners for copy/readback, cross-page seeks, reuse,
LRU/closed-entry eviction and shrink (`02e73c3d6`). Its final genuine original
completes in 1,041.236 ms; native takes 15.099 ms (setup 6.837, execution 6.214,
checks 2.048 ms). The ten-case/23-unit/original-build/159-graph checkpoint takes
0.605 s. Earlier marker reentry and a lost report-path receipt remain failed or
limited evidence; the final pair supplies the complete acceptance receipt.
The growth case uses real mapped and stream owners to write/read 4 MiB + 8,193
deterministic bytes, shrink to 2 MiB + 37 bytes and close both files
(`8a3e36831`). Both final hashes independently match the recipe. Original
completion takes 1,245.389 ms, native 239.704 ms (setup 4.815, execution 203.429,
checks 31.460 ms). All eleven cases, 23 units, the original build and full
161-graph/160-RET audit with the existing Release qualification pass in a
0.505 s incremental batch. No new shim,
provider or observer framework is needed. Full suites and commits occur at
case completion, rather than after each reconstructed function.

Case `0011` completes a genuine VC5 `/Zi` static-library archive
(`2a01eab26`), adding the 48-byte module-name function. The ordinary CLI pair
exits zero with identical packed DLL bytes and an unchanged compiler PDB:
original 698.662 ms, native 10.812 ms (setup 5.431, execution 3.743, checks
1.637 ms). Its dependency transcript is explicitly derived from immutable
case `0008`: only the App input binding changes; the actual dependency requests,
PDB bytes, provider premises, response bytes and lifetimes remain checked.
This does not claim another original dependency observation. Twelve cases,
23 units, the original build and full graph/RET gates pass in 2.060 s.

Case `0012` uses a valid format-authored PE32/NB05 input with wide C7 records,
two modules and acyclic field-list continuations (`0cf8cb702`). It is identified
as authored input, rather than historical compiler output; no stream is
signature-relabeled. Six real App bodies add 533 bytes. The untouched original
accepts the input, and the final ordinary CLI pair exits zero with 91 identical
entries and the same 4,096-byte packed DLL: original 1,359.216 ms, native
12.900 ms (setup 5.453, execution 6.185, checks 1.261 ms). All thirteen cached
cases, 23 units, the original compiler build and full 168-graph/167-RET gates
pass in 0.670 s. This case needs no Widen/PDB response or shared tool change.

RCDLL's rich-resource case completes at `f6cbd9acd`, including dialogs,
accelerators, icon/cursor/bitmap and raw resources, strings and nested version
information. The final ordinary original/native pair exits zero, has empty
streams and 217 identical first entries, and produces the same 3,672-byte RES:
original 839.197 ms, native 203.741 ms (setup 76.448, execution 57.043, checks
70.249 ms). Shared `7089c9add` closes the reached tracked-owner HeapDestroy
boundary using that saved original, without another original run. Both cached
cases, 92 unit groups and original/native builds pass in 0.553 s; strict
graph/ABI and SEH-role checks pass in 0.838 s. The 220-function/51,779-byte total
is the structured App registry's exact code union, excluding CRT startup
bodies; superseded intermediate tallies do not represent source or credit
changes. Original per-call private state remains qualified, rather than
universally observed. The worker is paused, with 52 known pending App records
(9,861 bytes) and 27 unclassified records (4,702 bytes), so a complete non-CRT
denominator is still unknown. Future font/RIFF/Mac resource or preprocessor
diagnostic workloads are proposals only; none has started.

LINK's hello EXE case completes at `89702eb2f`, with 304 App functions and
96,662 code bytes, or 23.30% of the 414,810-byte whole text extent. The ordinary
x86, nonincremental, no-debug case exits zero with matching entry order, streams
and a 24,064-byte executable. Final native replay takes 26.505 ms: setup 11.828,
execution 8.118 and checks 6.560 ms. Parallel native/original compiler builds
take 10.507 s; all four cached cases and 115 hello unit checks pass in 0.395 s.
The full 317-vertex graph/import/reference/ownership/extent audit, 308 ordinary
RET checks and both real C++ EH-role residuals pass in 1.406 s. The bounded
20-byte definition-buffer DIRECT prerequisite is included; optional domains
retain native guards or genuine pending traps. A reached diagnostic copy
corrected the modeled buffer prefix from 32 to 36 bytes with guards before
copies and concatenation; the original buffer's total capacity remains unknown.
Only the identified COFF timestamp DWORD is normalized, with its derived value
bound by the saved original's full output hash. Failed source/guard observations
remain preserved; these repairs add no function credit. The worker is paused.
There are 229 known pending App entries: 54 measured bodies total 8,216 bytes,
while 175 entry markers have unknown full extents. The registry is not a complete
binary inventory, so no full non-CRT denominator is claimed. A DLL case with
stdcall and DEF named/ordinal exports plus import-library output is the proposed
next substantial workload; it has not started.

C1's 7,458-byte global-record writer commits as `68ec306fc` only after original
and native builds, full code/data graph and ABI gates, 36 units and a fresh
758.922 ms original / 11.034 ms native pair. Its supported native domain is the
reached NULL/opcode-17/IL header; source-only symbol/non-IL fields stay explicitly
unmodeled behind the entry guard. C1XX's independently recovered larger
8,626-byte counterpart and subsequent frontend work remain uncommitted until
its current case completes; neither is included in the checkpoint table.
These large owners explain why function counts can stay flat during substantial
source work. Recent RCDLL menu-stage checks take 0.318 s for two native cases and
36 groups, compared with 11.348 s for original/native strict preflight; its
original/native pair takes 1.404 s / 188.450 ms. C2's latest original build and
audit take 3.016 s / 0.629 s, while native replay takes 11.013 ms.

CL completed pass configuration, the observed headless CONOUT$ failure path and
the typed inactive control handler. The final original handler exits with the
genuine status 4 and 59-entry cleanup trace (899.556 ms); the final native run
matches in 8.056 ms. All 18 cached cases, 18 unit invocations, original VC5 build
and graph/ABI gates pass in a fail-fast 3.744 s batch. An earlier cached-PASS
claim was wrong: missing descriptor trace fields failed, and a subsequent shell
command hid Make's exit status. That failed log is retained; the descriptor was
corrected and the complete batch passed before the final commit. No console
controller or post-exit assertion hook is retained. Public MRE helper reach,
console-success/pagination, delivered console events and active-DLL abort remain
outside the completed case claims.

LINK's module-symbol processor preserves its real C++ lifetime and two compiler
cleanup chunks under one App owner. Full rooted graph/ownership/import checks
remain strict; only this processor's aggregate RET assertion is replaced by a
small exact normal-RET24/action-RET0/handler-role check. All other scoped RET
checks remain active. A TU-specific native compiler setting eliminates the
same unused normal lifetime as the original while retaining the real outlined,
instrumented destructor for its genuine DIRECT case.

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

Further bounded storage corrections retain their earlier matching-state limits:
C1 removes a duplicate of the existing preprocess-stdout option at `54f28`
(`ee056a13a`); C1XX fixes two interner pool receivers and the token pool receiver
using the actual initializer addresses (`f4e3c5313`); LINK corrects an
entry-point-name versus entry-symbol predicate and tests distinct values
(`9a15f8d89`); C2 replaces a duplicate optimizer flag with the existing
`-bzalign` owner (`ac8675064`). None adds credit. Their current checks reuse
unchanged genuine originals; the C2 nonzero branch controls are explicitly
static, not executed-path claims. Newly encountered addresses are checked
against existing owners before declaring more storage.

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
- At an earlier checkpoint, CL's 91-byte console_rows and separate 62-byte
  control handler were still pending. The ordinary two-source warm-IDB MRE retry completed
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

CL's earlier bounded completion added the 41-byte deferred-job helper: 109 application
functions / 18,279 bytes. The genuine original DIRECT run passes in 1.328 s;
native passes in 7.661 ms (4.416 ms setup, 2.511 ms execution, 0.734 ms checks).
The original-emitted result and live ownership/link/ABI assertions cover NULL,
tail and head inputs. All 15 discovered cases (14 public IO plus one internal
DIRECT) and their tests pass in 0.602 s including case rebuilds; original build,
full 124-vertex audit and qualified 112-owner audit pass. This establishes
internal behavior, not ordinary public MRE reach. At that checkpoint the
remaining 266 App bytes were the console/control paths and pass toggle;
their subsequent qualified completion is recorded above.

CVPACK completed the released debug-DLL case and paused at `d967c21fc`, adding
11 application functions / 3,624 bytes. The final original exits successfully
in 1.045 s; native execution returns the same status and exact complete DLL
SHA-256 `935d7828c7003e4f951e79cb4a709c747eac8006405ac996299dea4e64dd0d40`
in 14.615 ms (5.339 ms setup, 7.852 ms execution, 1.424 ms checks). Both cached
cases, 13 narrow tests and the strict 124-function graph / 123-owner RET audit
pass in 0.287 s; final incremental native/original builds take 0.201/0.043 s.
The existing qualified dependency contract was unchanged. The autonomous
growth, catalog, diagnostic, telemetry and checksum work described above
followed that earlier pause.

## Watcom compatibility report

Shared integration is published. The simpler ordinary CF debugger is READY in
`cea53023405fcce7f9900f4626986008d8ded4db`, superseding the active instruction
controller from `04ee1337ad2f627d691b784f186d1d46b348da27`.
All five initial help cases are complete: WASM uses `/?`, and the other four
use bare `?`. All three CF binaries use the ordinary debugger; WLIB and WLINK
use the shared PE backend.

| Binary | Initial-case commit | App functions / unique original bytes | Exit |
| --- | --- | --- | --- |
| WCC386 | `8b50a144e` | 74 / 7,443 | 1 |
| WPP386 | `7bcf96442` | 98 / 6,827 | 1 |
| WASM | `f2710e44e` | 68 / 10,309 | 1 |
| WLIB | `23c416971` | 56 / 4,768 | 0 |
| WLINK | `5e182f985` | 116 / 8,962 | 0 |

All five match actual streams and entry order, pass period builds and full final
graph/ABI checks, and keep only `case.c`/`capture.bin` plus ignored build output
under cases. WCC's 74 logical identities include one seeded empty `CPragmaInit`
entry; WLINK's 116 include seeded `SetBreak`. Their actual observed entry counts
are 73 and 115; the shared empty entries add no unique instruction bytes and
their first-entry order is explicitly excluded. WCC, WPP and WLINK are paused
for user review. WLIB has autonomous substantial-case authorization. WASM also
has that authorization, but its current work is held after the platform
interruption described below.

The user subsequently approved integrating this backend into shared
`recon-tool original capture`. The shared recon-tool owner implements the
integration; the Watcom coordinator supplies controller contracts and review.
That release permits replacing the old minimal source/cases; history preserves
them. Workers use the shared runner, explicit original captures and period
compiler checks, with native-only regression fixtures. WLIB and WASM alone
have subsequent autonomous authorization.

**Initial reset plan (historical).**

| Binary | Historical accepted application work | Released first-case work |
| --- | --- | --- |
| WCC386 | 0 functions | Fresh scaffold and real native startup adapter; reuse the exact captured `47c77` frontier identity before proceeding. |
| WPP386 | 0 functions | Fresh scaffold and basic help; its own loader geometry and first App frontier remain to qualify. |
| WASM | 0 functions | Fresh scaffold and basic help; its own CF geometry and older CRT/AH=48h startup path remain to qualify. |
| WLIB | 2 functions / 143 bytes | Fresh scaffold and help; validated finite PE startup catalog. The current case uses explicit regular-file stdin. |
| WLINK | 1 function / 232 bytes | Fresh scaffold and help; validated finite PE startup catalog, with resource-reader conditions checked when reached. |

**Case-layout correction.** Per-function diagnostics had accumulated under cases,
contrary to the standardized layout. All five stopped generating them and
hash-preserved their raw histories in scratch before removing case copies and
unbound harness proof packages. A direct inventory confirmed every initial case
contains only `case.c`/`capture.bin` outside ignored `.build/`, with no legacy
nested evidence directories. Git retains historical tracked material. WCC
cached replay now reads descriptor
expectations and `capture.bin`, without a permanent original-report dependency;
fresh reconstruction comparisons still use the actual scratch original report.

The removed case witnesses were opaque documentary file/hash pins: their JSON
contents were not interpreted. Optional `runtime_evidence` was omitted without
changing registry CRT roles, extents or text data. Shared release `81cc675fb`
permits absent documentary evidence on canonical
identity, compatibility-call, shared-return and rebuilt-extent policies; supplied
bindings remain checked. CF release `e8f12177e` similarly removes the optional
nested layout documentary pin. Actual call sites, identities, CFG/ownership,
object/image/MAP and canonical CF profile/layout facts remain mandatory.
Clean-checkout audits do not require scratch history.

During migration WASM corrected an acceptance bookkeeping error: matching an
old trap did not prove execution of an anticipated candidate. At that point
its verified checkpoint was 46 App functions / 6,621 bytes at `ExpandTheWorld`,
with unsupported later candidates remasked. Subsequent genuine execution reached
the completed 68-function case above. No invalid oracle or commit was made. The acceptance check must prove
candidate body execution through to the next frontier or normal completion, not
merely a matching trap at the candidate entry.

**Completed-case measurements.** WLIB's final normal-exit original/native pair
was 487.759 / 8.974 ms; final cached check/unit were 8.098 / 10.516 ms, and
parallel period build plus final gates took 1.740 s. Its full 56-App graph,
references, ownership and extents pass; 54 ordinary return contracts plus a
finite residual cover two cold nonlocal tails without accepting pending recovery
code. WASM's final original/native pair was 1.342 s / 10.331 ms (5.572 setup,
2.712 execution, 2.047 checks); period rebuild plus audit took 2.783 s. Its full
68-App graph/reference/ownership/extent checks, 65 ordinary return contracts and
reviewed exceptional/help-exit residual pass.

WPP's final original/native pair takes 2,175.476 / 8.444 ms; cached replay is
8.195 ms (3.708 setup, 3.201 execution, 1.286 checks). Parallel native/period
builds take 1.572 s; its full audit target takes 153.08 ms for 98 App owners and
two justified CRT callback wrappers. A genuine file-scope const `Usage` data
public resolves the help table's rebuilt CODE boundary without a tool change.
WCC's final pair takes 1,573.763 / 7.772 ms; clean native/period/case-policy
builds take 3.320 s, full audit 161 ms for 77 paired owners (74 App plus three
justified CRT glue owners), and cached replay 7.874 ms (4.625 setup, 2.212
execution, 1.036 checks). Seventy ordinary App RET contracts and four finite
nonlocal-role residuals retain the actual exit/error boundaries. The canonical
7,443-byte union corrects an earlier eight-byte scratch-counter error; actual
registry extents were unchanged. Shared exporter `097fb522b` avoids parsing
MAP publics when numeric metadata needs no symbol resolution; real symbolic
requests, chunk checks and object-bound extent validation remain strict.
WLINK's final pair takes 1,414.433 / 19.600 ms (native 10.159 setup, 4.836
execution, 4.605 checks); the original launcher wall time is separately
3,399.974 ms. Exact stdout is 8,475 bytes, stderr/artifacts empty, exit 0.
Its final parallel period/native/fixture/unit/audit batch takes 1.265 s;
cached fixture/unit take 14.441 / 19.170 ms. All 116 App graphs, 115 RET
contracts and the explicit main-to-ExitProcess nonreturn residual pass.
No publication-only original was needed for these completed cases.

**Reached command-line input fix.** Shared release `61b2f237a` retains exact
`CreateProcessA` text independently of argv, binds mutable process-lifetime
native storage, and quotes only tokens that need escaping. This fixes a real
old-parser failure caused by always quoting safe `?`/`@response`/option tokens;
old raw captures are unchanged. WLINK's meaningful 42-entry pair passes at
`NewCommandSource`: original 1,217.919 ms / native 15.263 ms. The captured raw
program token is the requested R: path while observed Watcom argv0 is the staged
T: image, both retained honestly. WLIB's corrected library seed succeeds, and
its next substantial archive-update baseline exercises response/environment
options, replacement/deletion/extraction/addition, dictionary handling and an
explicit import. WASM's next coherent assembly case combines includes, macros,
conditionals, typed data, symbols/relocations and integer/x87 instructions.

**Subsequent cases, 26 October.** WLIB's substantial archive-update case is
committed as `250a712d5`: 171 verified App functions / 19,558 unique original
code bytes across the two completed cases. Its final archive invocation exits 0
with exact streams/artifacts and 166 observed first entries; the empty shared
`CommitDictionaryBlock` identity is seeded/excluded, adding no unique bytes.
The final pair takes 1,306.544 ms original / 13.635 ms native (7.903 setup,
3.183 execution, 2.550 checks), with incremental period/native build 1.094 s.
The final combined validation batch takes 2.585 s: cached help/archive replays
9.088 / 8.998 ms, period build, full 171-owner graph/ownership/extent checks,
165 strict RET contracts and six finite nonlocal-role residuals all pass.
Const demangler tables use the same genuine named data-public boundary solution
as WPP, without padding or a new tool feature. The current authorized substantial
workload combines in-place/backup replacement, C++ template/virtual/operator
symbols, COMDAT records, dictionary prime sizing above 36 publics, response
comments and member-name extraction. Genuine COMMON coverage is deferred;
record B7 is LPUBDEF, not LCOMDEF or COMDEF.

The in-place C++ case is complete in `c0338becb`: 213 qualified App functions /
24,667 unique original bytes, adding 42 functions and 5,109 bytes. Exit 0,
200 first entries, streams and complete archive/backup/demangled-listing/member
outputs match. The final pair takes 1,418.458 ms original / 22.565 ms native
(6.835 setup, 12.528 execution, 3.202 checks); cached replay takes 19.996 ms.
Changed-source period/native builds take 1.856 s and the final incremental
build 147 ms. All three cached cases and units pass, along with full
graph/reference/ownership/extent checks, 207 strict RET contracts and six
existing nonlocal residuals. Audit corrective runs take roughly 80–200 ms each;
the logs do not isolate a final tool-only total. Counts exclude the pending
one-byte `WresRecordError` placeholder.

Shared graph `c96d4f78f` validates exact terminal regions, including disconnected
fragments and inventoried existing CALL facts. The case's narrow 11-owner check
retains all 33 complete pair intersections and ordinary graph/ABI checks, without
changing masking or inferring callback contracts. Shared CRT `cbf2c1b5a` adds an
explicit WLIB opt-in for its identified older qsort algorithm: the real listing
exposes equal-key ordering, so the comparator and input remain unchanged. Its
retained original listing matches native in 19.888 ms without another original.
Existing unlink/rename suffice for the reached closed regular-file and absent
destination domain; existing-destination rename behavior remains unqualified.
Producer-only case sources were hash-preserved in scratch and removed by
`10fca91d0` / `894d3817d`; all three case directories now contain only `case.c`
and `capture.bin` outside generated `.build` output. No replay was needed for
that layout correction.

The PE-import/COMMON case is complete in `6520fbc82`: 227 qualified App
functions / 27,151 unique original bytes, adding 14 functions and 2,484 bytes.
The genuine period DLL has named/data exports at ordinals 1/7/11; this does not
claim ordinal-only export coverage. Valid authored near/far COMMON inputs expose
the 1994 decoder's short cursor advance for extended sizes. The complete caller
contract and actual next-name bytes confirm that behavior; reconstructed source
preserves it with a defect comment rather than changing input or masking output.
Standalone extended forms after the first desynchronization are not all claimed
as dynamically reached.

The fresh final pair matches 172 first entries, streams, artifacts and strict
final inventory in 1,412.100 ms original / 10.981 ms native (5.893 setup,
2.645 execution, 2.442 checks). Both internal cache files are removed normally.
An equivalent final in-function scan correction replays in 14.090 ms. All four
cached cases and units pass; full graph/reference/ownership/extent checks,
221 strict RET contracts and six existing residuals pass. Final parallel
native/period builds take 1.414 s and the full audit 310.588 ms.

Shared `ad369374a` supplies an actual native unique name and the reached
0261/0666 file-open tuple for the proved internal cache role. No captured PID
or historical filename equality is needed. Shared `e495bd9f8` explicitly selects
meaningful partial file assertions at identified frontiers while retaining all
raw artifacts; normal completion still uses full final inventory. Its actual
89-entry adoption takes 742.802 / 12.759 ms without a publication-only rerun.
The NE/NLM/operator case is complete in `7a5452e28`: 239 qualified App functions /
28,420 unique original bytes, adding 12 functions and 1,269 bytes. Genuine
NE resident/nonresident tables and a distinct NLM counted-export layout reach
their separate converters; no LE/LX coverage is claimed. Period C++ input
exercises assignment, relational and allocation operators, base32 integer
templates, recursive local names, based encoding and anonymous enums. The
209-entry final pair matches exact streams/artifacts and exit 0 in
1,384.099 ms original / 16.848 ms native (8.323 setup, 5.401 execution,
3.124 checks). All five cached cases and units pass in 96.343 ms wall;
parallel period/native builds take 2.628 s and full audit 399.460 ms for
239 owners, 233 strict RET contracts and six existing residuals. No new shared
support was needed.

All five case folders contain only `case.c` and `capture.bin` outside `.build`.
Twelve named App stubs remain, including member/syntax diagnostics, signal and
low-stack paths, suppression with no demonstrated CLI caller, and alternate
resource/error helpers. They remain App code without completion credit. The
next coherent member-resolution workload precedes a separate syntax/nonlocal
case: an early parser error cannot honestly also reach later member processing.

WASM's assembly case has reached 99 / 16,906 bytes at `AsmParse`; step042 is
authoritative.
That worker is temporarily held after a platform interruption, with WIP/raw
history preserved and no process running. The last completed command was a
successful read-only inspection; no action-specific cause is established.
WLINK's bounded live-Spawn `longjmp` adapter (`ce8c8d4df`) was adopted through
`Suicide` to `CloseSpillFile` in 1,139.107 ms original / 19.893 ms native before
the completed cleanup above. Windows SEH/expired-owner behavior remains outside
that finite native contract.

Shared CF raw-command input (`af8e6922e`) now records the actual loader
`CreateProcessA` buffer and independently observed payload tail. WPP's actual
92-entry `parseCmdLine` continuation passed in 2,350.712 ms original / 9.014 ms
native, with raw loader text distinct from the observed `?` tail. It was
adopted without a publication-only recapture. Reached `__STOSD`, `_splitpath2`,
private errno/OS storage, writable open and file-position support likewise use
shared implementations rather than local host shims.

WASM encountered an original read-before-buffer defect in `join_multiline_cmds`:
an empty line computes `line + strlen(line) - 1` before testing for a trailing
comma. Two permitted normal current-case observations both reached the
unambiguous no-join `AsmLine` frontier (1,612.908 / 1,293.007 ms original),
while native guards intentionally trapped (12.766 / 10.930 ms). These failed
comparisons earn no body credit. A documented native-only empty-line return
qualified through the next genuine matching pair to `AsmLine`, which then
executed to `ExpandMacro`. The Win32 defect and broad comment, blank-line and
conditional workload remain intact. No predecessor bytes were copied and no
stack layout was tuned.

**Earlier scaffold and committed migration checkpoints.**
WLINK scaffold commit `e469762a1` and WASM scaffold commit `207036122b` use the
shared build/runner and retire their local capture launchers. WASM's response
cleanup is committed as `e9836ce56`. WLINK and WASM now generate Watcom link
responses with Make's file function; cleanup required no extra original runs.
Current committed application checkpoints are WCC386 2 functions / 347 bytes
(`MemAlloc` and `MemInit`, `a0f0e8a2b`), WPP386 2 / 347 bytes (`a192b685f` and
`558cd1aca`), WLINK 22 / 2,361 bytes (resource header parsing, `f28888dac`), WLIB
6 / 546 bytes (through `FindResources`, `0be6fde0a`), and zero in WASM. WLIB's
additional 26-byte `res_seek` pair passes and awaits its acceptance checkpoint.
WCC's 20-byte `GetEnvVar` pair also passes; canonical READY releases promotion
using the unchanged successful capture/report. Built implementation flags and unaccepted
candidate counts are not acceptance. No complete code-coverage denominator is
claimed.
WCC's genuine native bootstrap consumes the real allocator result into its FILE
node list. Its new debugger pair reaches `SizeToClass 47b89`; WLINK now reaches
`readTypeList 436b60` after real resource-directory/header parsing. WLIB reaches
`InitResources 18ad8` after its genuine read/seek scan.
Shared release `b277c67f7` supports WCC's reached `ENTER 88,0` and finite WPP
geometry plus Make profile wiring. WPP's actual first-frontier qualification and
fresh native comparison now pass; complete help execution remains unfinished.
The old instruction controller rejected WCC's normal CRT `MOV ES,EAX`. The
replacement ordinary-debugger path now executes the original `getenv` unchanged
and passes the real WCC continuation: all four App entries match native and the
selected original report binds the next frontier. WCC/WPP/WASM profiles now use
binary-owned CODE/text-data layout without an instruction catalog. Canonical
release `cea530234` removes 31 obsolete controller/test/assets files and retains
the qualified debugger transport. All three canonical CF profile projections
match the reviewed candidate; WCC/WPP/WASM are reactivated for their initial
help cases. No further opcode/service-controller expansion is authorized.
WLIB/WLINK first-main captures are independent of resource-reader code that has
not executed. Later GS checks must address concrete reached values or missing
static proof, rather than preserve an old observer framework.

| Binary | Native measurement | Setup / execution / checks | Period compiler check |
| --- | --- | --- | --- |
| WCC386 | Ordinary-debugger GetEnvVar pair: native 5.716 ms; original 1.375 s (1.719 s wrapper) | 3.199 / 2.053 / 0.465 ms | Passed; candidate static graph and period build retained |
| WPP386 | Fresh MemAlloc pair: original 9.382 s; native command wall 22.5 ms | 2.725 / 2.004 / 0.423 ms | Passed; build plus audit 1.234 s. Later MemInit pair also passes; its native measurement includes GDB. |
| WASM | Authored scaffold frontier smoke: 4.889 ms | 2.599 / 2.020 / 0.270 ms | Passed; no timing claimed |
| WLIB | Real seek/read pair to InitResources: native 9.564 ms; original 836.569 ms | 5.875 / 1.862 / 1.827 ms | Passed; seven prospective App graphs and parallel build/audit 1.404 s |
| WLINK | Resource allocation pair to WResReadDir: native 18.335 ms; original 540.948 ms | Not separately reported | Passed; strict graph/RET/extents and main noreturn residual retained |

The measurements above are earlier checkpoints, not current function counts.
Authored scaffold smokes are not original-backed acceptance. WPP's first
App entry is its compiler allocator `81ba3`, size 8, before main. WASM differs:
its older CRT allocator reaches AH=48h before main, initially requesting 4,096
bytes. Its geometry and reached older CRT path now run through the simpler
ordinary debugger, which executes the original gateway normally. The WCC
65,536-byte App-allocator request does not qualify WASM's path. A WASM
435-byte heap member strongly correlates with shipped CRT code: all 32 differing
bytes are memory displacements smaller by four. This supports classification,
not byte identity, normalization or automatic admission. Native fixtures never
launch Wine; new original captures remain explicit.

Published `--watcom-main-argv` (`2d650edd0`) reads actual EAX argc / EDX argv
at the existing main stop, keeps requested and observed vectors separate, and
supplies the real argv0 to native IO. Exact self-image support (`cd6b47a7d`)
reconstructs the readonly staged image from its hash-bound source and compact
CC-only mask ranges outside the native cwd. WLIB and WLINK both qualify genuine
self-open/resource parsing with that input; no unmasked substitute or full image
blob is cached. Watcom's exact readonly binary open (`974310433`) and normal
regular-descriptor lseek (`657661533`) delegate to the existing descriptor
backend. Unknown flags/failures remain unsupported. The finite 496-byte
read/lseek/IOMode support closure is positively classified and shared between
these binaries with actual relocation/target comparison; cold errno/strerror
bodies remain masked. No API-only observer or new provider-identification engine
was needed.

Engineering time has dominated these run costs. WCC estimates 60–75% of recent
effort on scaffolding, startup classification and shared coordination, including
10–20 minutes waiting on shared fixes; its two fresh original captures totaled
19.456 seconds. WPP estimates 20–30 active minutes on migration/static setup and
WASM 30–55 minutes. Those earlier estimates preceded their subsequent
application acceptances. WLIB's
rough 50-minute account comprises 6 minutes application C, 14 scaffold, 9 heap
debugging, 13 audit integration and 8 queue wait. These are worker estimates,
not an aggregate measured time ledger. WLINK's recent main/Spawn work took about
six minutes plus subsequent source correlation; ordinary build/audit checks take
roughly 1.2–1.3 seconds. Avoidable work included overly broad future allocator
inventory, duplicate response helpers, path/link mistakes and repeated debugger
setup. Verified owner/transfer contracts should be reused; a new bespoke proof
campaign for each routine startup instruction is not a reconstruction requirement.

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
published. Reached API additions and their limits are recorded in the shared
changes table; no new PIC mechanism is required.

**1. Native migration prerequisites.** All five can use the current native
runner; it does not execute their original PE/CF format. Binary workers can move
application code to `src/`, glue to `harness/`, and use shared Make rules with
Watcom compile/link overrides. The user permits replacing the old minimal target
sources/cases; Git history suffices. Pending `InitMsg`/`Spawn` receives no credit
through that reset. CF output-only caches
must not be presented as complete trace or frontier oracles; missing entry
evidence stays explicit.

The minimum CF audit accommodations are published in the same shared release:

- `registry-export --image-base 0 --cf-profile harness/cf-identity.json` derives
  the finite CF image descriptor from an identity-only, project-relative profile.
  Loader/container/relocation hashes and base-zero coordinates are retained;
  graph-audit reads the real CF bytes. No synthetic PE or second CFG walker.
- Registry CRT roles, code extents and text data drive ordinary restoration.
  `runtime_evidence` is optional documentary provenance, not a replacement for
  classification. The old instruction catalog is retired. Historical
  original-only support exports retain `checked:null`; they do not invent
  counterpart bodies or ABI proof. No generic OMF matcher was added.

Entry-only pending App names remain zero-extent identities, never fabricated
one-byte owned bodies. Real pending stubs are not executable candidates. The
registry supplies the intended native trap symbol for each pending App, as ABI 2
requires, without thereby claiming its body or implementation.

DSO constructor ordering, WLIB's empty target-allocation namespace and actual
entry FP state need migration validation, rather than new shim semantics. Case
setup uses host allocation paths so it cannot falsely mark target activity.
Native GCC ABI is explicitly authorized; original Watcom builds retain true
watcall/signature/export checks. No per-function veneers or OMF-to-ELF transport
are needed. The exact WLINK-main noreturn residual and Watcom register ABI checks
remain target-specific; RET cleanup checks do not prove the whole ABI.
CF import and callback-table checks remain unsupported; their availability must
not be inferred from the existing CF reader or native typed-hook support.

**2. Original capture integration.** The user approved simplifying CF capture to
actual format/loading/relocation support, code/data-preserving masking and
ordinary debugger entry breakpoints. The original gateway executes under Wine;
no further hand-written opcode or gateway-service admission is planned. Existing
accepted application code and captures are retained. Static layout evidence
corroborates conservative CODE envelopes WCC `[0,0x704c4)`, WPP `[0,0xaad20)` and
WASM `[0,0x231b8)`, including embedded text data that must be preserved/restored.
These are recovered layout boundaries, not section fields stored in CF. Fine
CONST/DATA subdivision and a universal instruction inventory are unnecessary.
This simpler implementation passed the meaningful WCC GetEnvVar pair and is
canonically published as `cea530234`. The original records actual order
`47c77,47f15,66a2e,47b89`; native matches all four and traps at `47b89`.
The original report is bound by SHA-256
`638f94cbbac5da3b8205d4d4c7f105b537b3cd37071056cc53fc774127962b1b`.
Original elapsed time is 1,374.865 ms (1,226.292 ms setup, 2.106 ms between
ordinary resumes/stops); that execution-phase measurement is wall time, not CPU.
Separate frontend and waited-child CPU are 420.694 and 312.027 ms, excluding the
shared wineserver. The broker makes 2,559 RPCs, returns 0 and confirms owned
teardown without an outer kill. Native is 5.716 ms. No publication-only original
rerun is needed.

Shared `original capture --progress` currently
provides ordinary finite WCC frontier capture, App entry order and a bound partial
I/O oracle. Entry-stop remains diagnostic only. Both reuse the controller behind
the shared frontend with exact payload/loader identity, base-zero coordinates,
full relocation verification and separate target environment inputs. Mixed CF
code/data cannot be bulk-masked. WLIB and WLINK use the ordinary offline PE
masker with independently validated finite startup CRT/import catalogs; DGROUP,
resource overlays, file length and owned text data are retained. Reaching a callee
during startup alone does not classify it as CRT; application callbacks stay
masked. The old
bootstrap-then-mask/two-mode transport is not itself a requirement.
WLIB/WLINK/WASM patched originals retain exact GS-patch provenance. Before a
patched reader executes, establish its actual selector, capacity and owned-buffer
conditions using adequate static proof or a focused runtime assertion for an
unresolved reached value. A new persistent all-reader observer is not a blanket
prerequisite. Retain verified CRT bootstrap boundaries. Preserve actual stdin
semantics; WLIB's fresh regular-file input avoids requiring historical pipe
transport. Cached native replay does not invoke this capture path.
Fresh cases may explicitly capture staged argv0/self-open paths rather than
permanently retaining historical path quirks. Missing CF FP evidence stays
explicitly absent or unsupported; full DIRECT state is not a blanket initial
I/O migration prerequisite.

**Historical transport milestones.** The former controller preserved the original eight authored results under
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
`4d957bb5b`/`409be007c`. Publication removed all 199 tracked files under
`tooling/watcom-capture/` and the three obsolete WCC consumer scripts. Relevant
controls and finite fixtures now live under `tooling/recon-tool/tests/`; there
is no second active capture command. Historical evidence remains recoverable
in Git.

Finite static evidence now identifies WCC/WPP's three and WASM's four XI
callbacks as CRT using shipped-library bodies/fixups. Callback order is derived
statically, not observed. WASM's `main_init`, `ObjRecInit` and `GenMSOmfInit`
remain App code inside main. Original-only CRT structural export is now published
in `307e3ed37`: it supplies bound original instruction facts with no invented NT
counterpart, checked ABI or execution permission. Those exports and the former
per-instruction admission receipts are historical support facts, not an active
opcode permission requirement. In ordinary debugger mode the original CRT and
gateway execute directly; unknown code remains masked, while additional
processes/threads and CF DIRECT remain unsupported. Root separately released the
five initial-case workers.

**Shortest path to WCC's first real help frontier.** Static inspection found that
CRT FILE initialization calls the compiler's private allocator at CF offset
`47c77`, before main. Its source-correlated memory manager is application code,
not shipped CLIB `nmalloc`; the first real App call passes size 8 in EAX and
returns to `2f4d8`. No gateway service precedes that entry. The finite work and
remaining target-owned actions are:

| Work item | Owner | Status or next action |
| --- | --- | --- |
| Ordinary public CF capture | Shared recon-tool owner | Published: actual App entry/frontier/oracle path passed the public authored control and genuine WCC startup. |
| Five original support owners | WCC worker and shared owner | Qualified 397 bytes: `_cstart_` `2fe00..2feb4`, `__InitRtns` `48526..48571`, `_init_stk` `482c6..482ce`, `__InitFiles_` `4a1e5..4a25d`, allocation wrapper `2f4cd..2f4db`. Complete extents and execution subsets remain distinct. |
| Startup loading and layout | Shared recon-tool owner and binary workers | The ordinary debugger verifies the real loader allocation/relocations, preserves globals and encountered text data, restores positively classified CRT plus implemented App bodies, and stops at pending App entries. Original instructions and gateway services execute normally; no opcode implementation is needed. |
| Equivalent native startup | WCC worker | Implemented and paired: the target-specific `harness/` bootstrap performs the real callback and consumes its result into the FILE node list with proved order and state effects. |
| Later progress in the same help case | WCC worker | MemAlloc/MemInit already pass. Native harness/shims reproduce the real allocation effects, while the original loader executes AH=48h/LocalAlloc itself. Continue ordinary App reconstruction; classify concrete newly reached CRT bodies without inventing a service interpreter. |

The wrapper's LEAVE and the memory-sourced startup stack reservation occur after
the first allocator call returns; they are later progression requirements, not
prerequisites for the first frontier. The exact five-owner witness, including
bytes, CF fixups, return contracts and first-entry effects, is
`~/.ai-tmp/msvc2/watcom-shared-integration/wcc-startup/first-frontier.json`
(SHA-256 `d29b80632db9a5f9b30fc48dcf703e98ac205e0d613288cf514cb316aedcfa0e`).
The actual loader service mapping is separately documented in
`wcc-gateway-minimum/summary.md` under the same scratch root. These static packets
and planned adaptations confer no application acceptance. The released workers
must obtain fresh matching evidence as their initial case progresses, and pause
after that case is complete. Concrete shared gaps are reported to the existing
shared owners, without local replacement capture tools or startup waivers.

The root-approved genuine `?` qualification stopped at actual `MemAlloc 47c77`
with EAX 8, ECX pointing to the first FILE, EDX 1 and real stack return `2f4d8`.
Only App entry `[47c77]` was observed; no allocator body or gateway ran. It took
9.042 s guarded / 11.082 s overall for 10,848 steps and 73,783 RPCs. The actual
2,058-byte environment required a conservative 18,695-step allowance within the
32,768 cap. Adaptive polling reduced normal RPC medians, with a CPU tradeoff:
backend frontend CPU 9.774 s and its waited-child CPU 5.895 s; shared wineserver
CPU is excluded. The authored public 40-step/four-return control took 1.655 s
overall (37.179 ms guarded), and the finite startup-op control took 1.687 s
(52.781 ms guarded). No overall speedup claim is made from differing loader waits.

The genuine capture/report remain unchanged and reusable only with their exact
`wcc-first-help-frontier` / `WCC386.EXE` / IO-0 / empty-seen case and input identity.
The binary-owned native bootstrap now reproduces the real callback and state
effects through `--original-report`; publication required no repeat original run.
WPP/WASM layouts project through the shared exporter; their next actual pairs
must still qualify the real original/native continuation. CF DIRECT and complete
help execution remain unqualified. Canonical controls pass; READY receipt is
`~/.ai-tmp/msvc2/watcom-ordinary-capture-20261025/READY.json` (SHA-256
`463c1de1773c77406dddc60fc9cafc5ff6f84d3bb968f525c2d96bdff6b57cbe`).

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
records support needs, bounded transport milestones and fresh application
acceptances. WLIB and WASM initial help cases are complete; the other three
remain in progress.
